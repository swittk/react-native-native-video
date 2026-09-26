#include <jni.h>

#include <memory>
#include <string>

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>

#include "SKAndroidNativeVideoCPP.h"
#include "react-native-native-video.h"

extern "C" JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_installLegacyBindings(
    JNIEnv *env,
    jobject module,
    jlong runtimePointer,
    jobject callInvokerHolderObject) {
  if (runtimePointer == 0 || callInvokerHolderObject == nullptr) {
    return;
  }

  auto state = std::make_shared<SKRNNativeVideo::AndroidModuleState>(env, module);
  auto holder = facebook::jni::alias_ref<
      facebook::react::CallInvokerHolder::javaobject>(
      reinterpret_cast<facebook::react::CallInvokerHolder::javaobject>(
          callInvokerHolderObject));
  auto callInvoker = holder->cthis()->getCallInvoker();
  auto *runtime = reinterpret_cast<facebook::jsi::Runtime *>(runtimePointer);

  callInvoker->invokeAsync([state = std::move(state), runtime]() {
    SKRNNativeVideo::install(
        *runtime,
        [state](
            facebook::jsi::Runtime &,
            const std::string &path) {
          return std::make_shared<
              SKRNNativeVideo::SKAndroidNativeVideoWrapper>(path, state);
        });
  });
}
