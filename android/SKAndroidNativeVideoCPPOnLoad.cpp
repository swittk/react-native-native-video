//
// Created by Switt Kongdachalert on 24/1/2022 AD.
//
#include <jni.h>

namespace SKRNNativeVideo {
    extern jclass NativeVideoWrapperJavaSideClass;
    extern jmethodID NativeVideoWrapperJavaGetFrameAtIndexMethod;
    extern jmethodID NativeVideoWrapperJavaGetFramesAtIndexMethod;
    extern jmethodID NativeVideoWrapperJavaGetFrameAtTimeMethod;
    extern jmethodID NativeVideoWrapperJavaGetNumFramesMethod;
    extern jmethodID NativeVideoWrapperJavaGetFrameRateMethod;
    extern jmethodID NativeVideoWrapperJavaGetDurationMethod;
    extern jmethodID NativeVideoWrapperJavaGetWidthMethod;
    extern jmethodID NativeVideoWrapperJavaGetHeightMethod;
    extern jmethodID NativeVideoWrapperJavaSideClassConstructor;
    extern jmethodID NativeVideoWrapperJavaSideBase64ForBitmapMethod;
    extern jmethodID NativeVideoWrapperJavaSideRGBABytesForBitmapMethod;

    extern jmethodID BitmapGetWidthMethod;
    extern jmethodID BitmapGetHeightMethod;
    extern jclass BitmapClassRef;

    extern jclass java_util_List;
    extern jmethodID java_util_List_;
    extern jmethodID java_util_List_size;
    extern jmethodID java_util_List_get;
    extern jmethodID java_util_List_add;

}

using namespace SKRNNativeVideo;
// I moved JNI_Onload over to this file in hopes of making JNI_OnLoad not clash with other projects.. hopefully it works.

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

    NativeVideoWrapperJavaSideRGBABytesForBitmapMethod = env->GetStaticMethodID(SKNativeVideoCLS, "rgbaValuesFromBitmap", "(Landroid/graphics/Bitmap;)[B");

    BitmapClassRef = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Bitmap"));
    BitmapGetWidthMethod = env->GetMethodID(BitmapClassRef, "getWidth", "()I");
    BitmapGetHeightMethod = env->GetMethodID(BitmapClassRef, "getHeight", "()I");

    return JNI_VERSION_1_6;
}
