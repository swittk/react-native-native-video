//
// Created by Switt Kongdachalert on 5/1/2022 AD.
//

#include "SKAndroidNativeVideoCPP.h"
#include <jni.h>

using namespace SKRNNativeVideo;
namespace SKRNNativeVideo {
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
    jmethodID NativeVideoWrapperJavaSideBase64ForBitmapMethod = 0; // Static
    jmethodID NativeVideoWrapperJavaSideRGBABytesForBitmapMethod = 0; // Static

    jmethodID BitmapGetWidthMethod = 0;
    jmethodID BitmapGetHeightMethod = 0;
    jclass BitmapClassRef = 0;

    jclass java_util_List;
    jmethodID java_util_List_;
    jmethodID java_util_List_size;
    jmethodID java_util_List_get;
    jmethodID java_util_List_add;
}

static std::string jstring2string(JNIEnv *env, jstring jStr);

// Adapted from here https://stackoverflow.com/a/16668081/4469172
// Don't forget to free()!
static unsigned char* jbyteArray_createUnsignedCharArray(JNIEnv *env, jbyteArray array) {
    int len = env->GetArrayLength (array);
    unsigned char* buf = (unsigned char *)malloc(len);
    env->GetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
    return buf;
}
static jbyteArray jbyteArray_createFromByteArray(JNIEnv *env, unsigned char* buf, int len) {
    jbyteArray array = env->NewByteArray (len);
    env->SetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
    return array;
}

// TODO: I have no idea if JNIEnv * is safe to be stored in classes; if anyone knows better please make pull requests or something
SKAndroidNativeVideoWrapper::SKAndroidNativeVideoWrapper(
        std::string sourceUri,
        JavaVM* _vm
) : SKNativeVideoWrapper(sourceUri), jvm(_vm) {
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
    return std::make_shared<SKAndroidNativeFrameWrapper>(jvm, bitmap);
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
        ret.push_back(std::make_shared<SKAndroidNativeFrameWrapper>(jvm, bitmap));
    }
    clearJNIEnv();
    return ret;
};
std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtTime(double time) {
    JNIEnv *env = getJNIEnv();
    jobject bitmap = env->CallObjectMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameAtTimeMethod, time);
    clearJNIEnv();
    return std::make_shared<SKAndroidNativeFrameWrapper>(jvm, bitmap);
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

SKAndroidNativeFrameWrapper::SKAndroidNativeFrameWrapper(JavaVM *_vm, jobject _bitmap) :
SKNativeFrameWrapper(), jvm(_vm)
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

facebook::jsi::Value SKAndroidNativeFrameWrapper::arrayBufferValue(facebook::jsi::Runtime &runtime) {
     using namespace facebook;
     JNIEnv *env = getJNIEnv();
    jbyteArray arr = (jbyteArray)env->CallStaticObjectMethod(NativeVideoWrapperJavaSideClass, NativeVideoWrapperJavaSideRGBABytesForBitmapMethod, bitmap);
    int len = env->GetArrayLength(arr);
    unsigned char *bytes = jbyteArray_createUnsignedCharArray(env, arr);
    jsi::Function arrayBufferCtor = runtime.global().getPropertyAsFunction(runtime, "ArrayBuffer");
    size_t totalBytes = len;
    jsi::Object o = arrayBufferCtor.callAsConstructor(runtime, jsi::Value((int)totalBytes)).getObject(runtime);
    jsi::ArrayBuffer buf = o.getArrayBuffer(runtime);
    memcpy(buf.data(runtime), bytes, totalBytes);
    free(bytes);
    return std::move(o);
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
