#include <jni.h>

#include "SKAndroidNativeVideoCPP.h"

#if RNNATIVEVIDEO_NEW_ARCH
#include <fbjni/fbjni.h>

#include "NativeVideoBindings.h"
#endif

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
#if RNNATIVEVIDEO_NEW_ARCH
  return facebook::jni::initialize(vm, [] {
    SKRNNativeVideo::initializeJavaBindings(
        facebook::jni::Environment::current());
    SKRNNativeVideo::NativeVideoBindings::registerNatives();
  });
#else
  JNIEnv *env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK ||
      env == nullptr) {
    return JNI_ERR;
  }
  SKRNNativeVideo::initializeJavaBindings(env);
  return JNI_VERSION_1_6;
#endif
}
