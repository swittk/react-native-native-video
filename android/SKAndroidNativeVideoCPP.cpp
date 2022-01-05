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
    jclass NativeVideoWrapperJavaSideClass = env->FindClass(
            "com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
    printf("found class");
    jmethodID constructorMethod = env->GetMethodID(NativeVideoWrapperJavaSideClass, "<init>",
                                                                   "(Ljava/lang/String;)V");
    printf("got constructor");
    // The MediaMetadataRetriever wrapper object
    jstring stringUri = env->NewStringUTF(sourceUri.c_str());
    jobject obj = env->NewObject(NativeVideoWrapperJavaSideClass, constructorMethod, stringUri);
    printf("got jobj");
    javaVideoWrapper = env->NewGlobalRef(obj);
    printf("got javavideowrapper");
    setValid(true);
    clearJNIEnv();
//    env->Call
//    env->GetObjectClass();
}

void SKAndroidNativeVideoWrapper::close() {
    // TODO: Cleanup
    JNIEnv *env = getJNIEnv();
    env->DeleteGlobalRef(javaVideoWrapper);
    clearJNIEnv();
}

std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtIndex(int index) {
    JNIEnv *env = getJNIEnv();
    // According to https://stackoverflow.com/a/2093300/4469172, I should not reuse jclass, but `jmethodID`s are reusable.
    jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
    if(!NativeVideoWrapperJavaGetFrameAtIndexMethod) {
        NativeVideoWrapperJavaGetFrameAtIndexMethod = env->GetMethodID(MyClass, "getFrameAtIndex",
                                                                       "(I)Landroid/graphics/Bitmap;");
    }
//    jmethodID meth = env->GetMethodID(MyClass, "getFrameAtIndex",
//                                      "(I)Landroid/graphics/Bitmap;");
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
    jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
    if(!NativeVideoWrapperJavaGetFramesAtIndexMethod) {
        NativeVideoWrapperJavaGetFramesAtIndexMethod = env->GetMethodID(MyClass, "getFramesAtIndex",
                                                                       "(II)Ljava/util/List;");
    }
    jmethodID  meth = env->GetMethodID(MyClass, "getFramesAtIndex",
                                                                    "(II)Ljava/util/List;");
    jobject listObj = env->CallObjectMethod(javaVideoWrapper, meth, index, len);
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
    jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
    if(!NativeVideoWrapperJavaGetFrameAtTimeMethod) {
        NativeVideoWrapperJavaGetFrameAtTimeMethod = env->GetMethodID(MyClass, "getFrameAtTime",
                                                                       "(D)Landroid/graphics/Bitmap;");
    }
//    jmethodID meth = env->GetMethodID(MyClass, "getFrameAtTime",
//                                      "(D)Landroid/graphics/Bitmap;");
    jobject bitmap = env->CallObjectMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameAtTimeMethod, time);
    clearJNIEnv();
    return std::make_shared<SKAndroidNativeFrameWrapper>(runtime, jvm, bitmap);
};
 int SKAndroidNativeVideoWrapper::numFrames() {
     JNIEnv *env = getJNIEnv();
     jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
     if(!NativeVideoWrapperJavaGetNumFramesMethod) {
         NativeVideoWrapperJavaGetNumFramesMethod = env->GetMethodID(MyClass, "getNumFrames",
                                                                       "()I");
     }
     int res = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetNumFramesMethod);
     clearJNIEnv();
     return res;
 };

 double SKAndroidNativeVideoWrapper::frameRate() {
     JNIEnv *env = getJNIEnv();
     jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
     if(!NativeVideoWrapperJavaGetFrameRateMethod) {
         NativeVideoWrapperJavaGetFrameRateMethod = env->GetMethodID(MyClass, "getFrameRate",
                                                                     "()D");
     }
     double ret = env->CallDoubleMethod(javaVideoWrapper, NativeVideoWrapperJavaGetFrameRateMethod);
     clearJNIEnv();
     return ret;
 };

 SKRNSize SKAndroidNativeVideoWrapper::size() {
     JNIEnv *env = getJNIEnv();
     jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
     if(!NativeVideoWrapperJavaGetWidthMethod) {
         NativeVideoWrapperJavaGetWidthMethod = env->GetMethodID(MyClass, "getWidth",
                                                                     "()I");
     }
     if(!NativeVideoWrapperJavaGetHeightMethod) {
         NativeVideoWrapperJavaGetHeightMethod = env->GetMethodID(MyClass, "getHeight",
                                                                 "()I");
     }
     int width = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetWidthMethod);
     int height = env->CallIntMethod(javaVideoWrapper, NativeVideoWrapperJavaGetHeightMethod);
     clearJNIEnv();
     return (SKRNSize) {.width=(double)width, .height=(double)height};
 }

 double SKAndroidNativeVideoWrapper::duration() {
     JNIEnv *env = getJNIEnv();
     jclass MyClass = env->FindClass("com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
     if(!NativeVideoWrapperJavaGetDurationMethod) {
         NativeVideoWrapperJavaGetDurationMethod = env->GetMethodID(MyClass, "getDuration",
                                                                    "()D");
     }
     double ret = env->CallDoubleMethod(javaVideoWrapper, NativeVideoWrapperJavaGetDurationMethod);
     clearJNIEnv();
     return ret;
 }


void SKAndroidNativeVideoWrapper::initListHelper(JNIEnv *env) {
    java_util_List      = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/List")));
    java_util_List_     = env->GetMethodID(java_util_List, "<init>", "()V");
    java_util_List_size = env->GetMethodID (java_util_List, "size", "()I");
    java_util_List_get  = env->GetMethodID(java_util_List, "get", "(I)Ljava/lang/Object;");
    java_util_List_add  = env->GetMethodID(java_util_List, "add", "(Ljava/lang/Object;)Z");
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

void SKAndroidNativeFrameWrapper::close() {
    JNIEnv *env = getJNIEnv();
    env->DeleteGlobalRef(bitmap);
    clearJNIEnv();
}

SKRNSize SKAndroidNativeFrameWrapper::size() {
    JNIEnv *env = getJNIEnv();
    jclass bitmapclass = env->FindClass("android/graphics/Bitmap");
    jmethodID widthGet = env->GetMethodID(bitmapclass, "getWidth", "()I");
    jmethodID heightGet = env->GetMethodID(bitmapclass, "getHeight", "()I");
    int width = env->CallIntMethod(bitmap, widthGet);
    int height = env->CallIntMethod(bitmap, heightGet);
//    env->CallDoubleMethod(bitmap, bitmapclass, "" )
    clearJNIEnv();
    return (SKRNSize){(double)width, (double)height};
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
