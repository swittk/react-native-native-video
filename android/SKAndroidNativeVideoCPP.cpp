//
// Created by Switt Kongdachalert on 5/1/2022 AD.
//

/*
 * Tips to self :
 * JVM's AttachCurrentThread MUST be balanced with a DetachCurrentThread or I'll get headaches like
 * spending all day of 05/01/2022 on (memory access to NULL, EACCESS, etc.).
 * See similar issue : https://stackoverflow.com/a/26534926/4469172
 *
 */

#include "SKAndroidNativeVideoCPP.h"
#include <jni.h>

jclass NativeVideoWrapperJavaSideClass = 0;
jmethodID NativeVideoWrapperJavaGetFrameAtIndexMethod = 0;
jmethodID NativeVideoWrapperJavaGetFramesAtIndexMethod = 0;
jmethodID NativeVideoWrapperJavaGetFrameAtTimeMethod = 0;
jmethodID NativeVideoWrapperJavaGetNumFramesMethod = 0;
jmethodID NativeVideoWrapperJavaGetFrameRateMethod = 0;
jmethodID NativeVideoWrapperJavaGetDurationMethod = 0;
jmethodID NativeVideoWrapperJavaGetWidthMethod = 0;
jmethodID NativeVideoWrapperJavaGetHeightMethod = 0;
jmethodID NativeVideoWrapperJavaSideClassConstructor = 0;
jmethodID NativeVideoWrapperJavaSideBase64ForBitmapMethod = 0;

jmethodID BitmapGetWidthMethod = 0;
jmethodID BitmapGetHeightMethod = 0;
jclass BitmapClassRef = 0;

jclass java_util_List;
jmethodID java_util_List_;
jmethodID java_util_List_size;
jmethodID java_util_List_get;
jmethodID java_util_List_add;

using namespace SKRNNativeVideo;

static std::string jstring2string(JNIEnv *env, jstring jStr);

// TODO: I have no idea if JNIEnv * is safe to be stored in classes; if anyone knows better please make pull requests or something
SKAndroidNativeVideoWrapper::SKAndroidNativeVideoWrapper(
        facebook::jsi::Runtime &runtime,
        std::string sourceUri,
        JavaVM* _vm
) : SKNativeVideoWrapper(runtime, sourceUri), jvm(_vm) {
    printf("about to init listHelper");
    printf("about to get jnienv");
    JNIEnv *env = getJNIEnv();
    initListHelper(env);
    printf("got jnienv %d", env);
    printf("found class");
    printf("got constructor");
    // The MediaMetadataRetriever wrapper object
    jstring stringUri = env->NewStringUTF(sourceUri.c_str());
    jobject obj = env->NewObject(NativeVideoWrapperJavaSideClass, NativeVideoWrapperJavaSideClassConstructor, stringUri);
    printf("got jobj");
    javaVideoWrapper = env->NewGlobalRef(obj);
    printf("got javavideowrapper");
    setValid(true);
    clearJNIEnv();
//    env->Call
//    env->GetObjectClass();
}
SKAndroidNativeVideoWrapper::~SKAndroidNativeVideoWrapper() {
    close();
}

void SKAndroidNativeVideoWrapper::close() {
    if(javaVideoWrapper != nullptr) {
        // TODO: Cleanup
        JNIEnv *env = getJNIEnv();
        setValid(false);
        env->DeleteGlobalRef(javaVideoWrapper);
        javaVideoWrapper = nullptr;
        clearJNIEnv();
    }
}

std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtIndex(int index) {
    JNIEnv *env = getJNIEnv();
    // According to https://stackoverflow.com/a/2093300/4469172, I should not reuse jclass, but `jmethodID`s are reusable.
    jobject bitmap = env->CallObjectMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameAtIndexMethod, index);
    clearJNIEnv();
    return std::make_shared<SKAndroidNativeFrameWrapper>(runtime, jvm, bitmap);
};

std::vector<std::shared_ptr<SKNativeFrameWrapper>>
SKAndroidNativeVideoWrapper::getFramesAtIndex(int index, int len) {
    JNIEnv *env = getJNIEnv();
    std::vector<std::shared_ptr<SKNativeFrameWrapper>> ret = std::vector<std::shared_ptr<SKNativeFrameWrapper>>();
    // According to https://stackoverflow.com/a/2093300/4469172, I should not reuse jclass, but `jmethodID`s are
    // reusable.
    jobject listObj = env->CallObjectMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFramesAtIndexMethod, index, len);
    std::vector<jobject> bitmaps = javaList2vector_jobjects(env, listObj);
    for(jobject bitmap : bitmaps) {
        ret.push_back(std::make_shared<SKAndroidNativeFrameWrapper>(runtime, jvm, bitmap));
    }
    clearJNIEnv();
    return ret;
};
std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtTime(double time) {
    JNIEnv *env = getJNIEnv();
    jobject bitmap = env->CallObjectMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameAtTimeMethod, time);
    clearJNIEnv();
    return std::make_shared<SKAndroidNativeFrameWrapper>(runtime, jvm, bitmap);
};
 int SKAndroidNativeVideoWrapper::numFrames() {
     JNIEnv *env = getJNIEnv();
     int res = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetNumFramesMethod);
     clearJNIEnv();
     return res;
 };

 double SKAndroidNativeVideoWrapper::frameRate() {
     JNIEnv *env = getJNIEnv();
     double ret = env->CallDoubleMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameRateMethod);
     clearJNIEnv();
     return ret;
 };

 SKRNSize SKAndroidNativeVideoWrapper::size() {
     JNIEnv *env = getJNIEnv();
     int width = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetWidthMethod);
     int height = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetHeightMethod);
     clearJNIEnv();
     return (SKRNSize) {.width=(double)width, .height=(double)height};
 }

 double SKAndroidNativeVideoWrapper::duration() {
     JNIEnv *env = getJNIEnv();
     double ret = env->CallDoubleMethod(javaVideoWrapper, NativeVideoWrapperJavaGetDurationMethod);
     clearJNIEnv();
     return ret;
 }


void SKAndroidNativeVideoWrapper::initListHelper(JNIEnv *env) {
}

// Adapted from https://stackoverflow.com/a/33408920/4469172
std::vector<jobject> SKAndroidNativeVideoWrapper::javaList2vector_jobjects(JNIEnv *env, jobject arrayList) {
    jint len = env->CallIntMethod(arrayList, java_util_List_size);
    std::vector<jobject> result;
    result.reserve(len);
    for (jint i=0; i<len; i++) {
        jobject element = static_cast<jobject>(env->CallObjectMethod(arrayList, java_util_List_get, i));
        result.push_back(element);
    }
    return result;
}




#pragma mark - FrameWrapper methods

SKAndroidNativeFrameWrapper::SKAndroidNativeFrameWrapper(facebook::jsi::Runtime &_runtime, JavaVM *_vm, jobject _bitmap) :
SKNativeFrameWrapper(_runtime), jvm(_vm)
{
     JNIEnv *env = getJNIEnv();
    if(_bitmap == NULL) {
        setValid(false);
        clearJNIEnv();
        return;
    }

    bitmap = env->NewGlobalRef(_bitmap);

    setValid(true);
    // TODO: DO MORE
    clearJNIEnv();
}
SKAndroidNativeFrameWrapper::~SKAndroidNativeFrameWrapper() {
     close();
}

void SKAndroidNativeFrameWrapper::close() {
    if(bitmap != nullptr) {
        JNIEnv *env = getJNIEnv();
        setValid(false);
        env->DeleteGlobalRef(bitmap);
        bitmap = nullptr;
        clearJNIEnv();
    }
}

SKRNSize SKAndroidNativeFrameWrapper::size() {
    JNIEnv *env = getJNIEnv();
    int width = env->CallIntMethod(bitmap, BitmapGetWidthMethod);
    int height = env->CallIntMethod(bitmap, BitmapGetHeightMethod);
//    env->CallDoubleMethod(bitmap, bitmapclass, "" )
    clearJNIEnv();
    return (SKRNSize){(double)width, (double)height};
}

std::string SKAndroidNativeFrameWrapper::base64(std::string format) {
    JNIEnv *env = getJNIEnv();
//    jclass SKNativeVideoCLS = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
//    jmethodID strForBitmap = env->GetStaticMethodID(SKNativeVideoCLS, "Base64StringForBitmap", "(Landroid/graphics/Bitmap;Ljava/lang/String;)Ljava/lang/String;");
    // Call the Java side to do the conversion.
    jstring retStr = (jstring)env->CallStaticObjectMethod(NativeVideoWrapperJavaSideClass, NativeVideoWrapperJavaSideBase64ForBitmapMethod, bitmap, env->NewStringUTF(format.c_str()));
    return std::string(env->GetStringUTFChars(retStr, 0));
}


extern "C"
JNIEXPORT jobject JNICALL
Java_com_reactnativenativevideo_SKRNNativeFrameViewManager_getBitmapFromStringAddressOfFrameWrapper(
        JNIEnv *env, jclass clazz, jstring address) {
     std::string addrString = jstring2string(env, address);
    SKAndroidNativeFrameWrapper *wrapper = (SKAndroidNativeFrameWrapper *)StringToPointer(addrString);
    if(wrapper == NULL) {
        return nullptr;
    }
    return wrapper->bitmap;
}


static std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

// Following this great example here
// https://github.com/rdixonbhw/ReactNative-JNI-Blog/blob/master/android/app/src/main/jni/hello_world.c
// Which can be cross-referenced here https://thebhwgroup.com/blog/react-native-jni
extern "C"
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if(vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    java_util_List      = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/List")));
    java_util_List_     = env->GetMethodID(java_util_List, "<init>", "()V");
    java_util_List_size = env->GetMethodID (java_util_List, "size", "()I");
    java_util_List_get  = env->GetMethodID(java_util_List, "get", "(I)Ljava/lang/Object;");
    java_util_List_add  = env->GetMethodID(java_util_List, "add", "(Ljava/lang/Object;)Z");

    jclass SKNativeVideoCLS = (jclass)env->NewGlobalRef(env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide"));
    NativeVideoWrapperJavaSideClass = SKNativeVideoCLS;
    NativeVideoWrapperJavaGetDurationMethod = env->GetMethodID(SKNativeVideoCLS, "getDuration", "()D");
    NativeVideoWrapperJavaGetFrameRateMethod = env->GetMethodID(SKNativeVideoCLS, "getFrameRate", "()D");
    NativeVideoWrapperJavaGetWidthMethod = env->GetMethodID(SKNativeVideoCLS, "getWidth","()I");
    NativeVideoWrapperJavaGetNumFramesMethod = env->GetMethodID(SKNativeVideoCLS, "getNumFrames","()I");
    NativeVideoWrapperJavaGetFrameAtIndexMethod = env->GetMethodID(SKNativeVideoCLS, "getFrameAtIndex",
                                                                   "(I)Landroid/graphics/Bitmap;");
    NativeVideoWrapperJavaGetFramesAtIndexMethod = env->GetMethodID(SKNativeVideoCLS, "getFramesAtIndex",
                                                                    "(II)Ljava/util/List;");
    NativeVideoWrapperJavaGetFrameAtTimeMethod = env->GetMethodID(SKNativeVideoCLS, "getFrameAtTime",
                                                                  "(D)Landroid/graphics/Bitmap;");
    NativeVideoWrapperJavaGetHeightMethod = env->GetMethodID(SKNativeVideoCLS, "getHeight",
                                                             "()I");
    NativeVideoWrapperJavaSideClassConstructor = env->GetMethodID(SKNativeVideoCLS, "<init>",
                                                                  "(Ljava/lang/String;)V");
    NativeVideoWrapperJavaSideBase64ForBitmapMethod = env->GetStaticMethodID(SKNativeVideoCLS, "Base64StringForBitmap", "(Landroid/graphics/Bitmap;Ljava/lang/String;)Ljava/lang/String;");

    BitmapClassRef = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Bitmap"));
    BitmapGetWidthMethod = env->GetMethodID(BitmapClassRef, "getWidth", "()I");
    BitmapGetHeightMethod = env->GetMethodID(BitmapClassRef, "getHeight", "()I");

    return JNI_VERSION_1_6;
}
