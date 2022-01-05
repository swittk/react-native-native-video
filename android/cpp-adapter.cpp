#include <jni.h>
#include "react-native-native-video.h"
#include "SKAndroidNativeVideoCPP.h"
#include <memory>


extern "C"
JNIEXPORT jint JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_nativeMultiply(JNIEnv *env, jclass type, jint a, jint b) {
    return SKRNNativeVideo::multiply(a, b);
}

using namespace SKRNNativeVideo;
extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_initialize(JNIEnv *env, jclass clazz,
                                                             jlong jsi_runtime_pointer) {
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    printf("javaVM was %d", jvm);
    SKRNNativeVideo::install(
            *reinterpret_cast<facebook::jsi::Runtime *>(jsi_runtime_pointer),
            [&, jvm](facebook::jsi::Runtime &runtime,
               std::string path) -> std::shared_ptr<SKNativeVideoWrapper> {
                std::shared_ptr<SKNativeVideoWrapper> ret = std::make_shared<SKAndroidNativeVideoWrapper>(
                        runtime, path, jvm);
                return ret;
            }
    );
}
extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_cleanup(JNIEnv *env, jclass clazz,
                                                          jlong jsi_runtime_pointer) {
    // TODO: Maybe do something here
}
