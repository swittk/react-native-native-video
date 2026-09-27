#include <jni.h>

#include <memory>
#include <string>

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>

#include "SKAndroidNativeVideoCPP.h"
#include "react-native-native-video.h"

namespace {

void installLegacyRuntime(
    facebook::jsi::Runtime &runtime,
    std::shared_ptr<SKRNNativeVideo::AndroidModuleState> state) {
  SKRNNativeVideo::install(
      runtime,
      [state = std::move(state)](
          facebook::jsi::Runtime &,
          const std::string &path) {
        return std::make_shared<
            SKRNNativeVideo::SKAndroidNativeVideoWrapper>(path, state);
      });
}

} // namespace

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

  callInvoker->invokeAsync([state = std::move(state), runtime]() mutable {
    installLegacyRuntime(*runtime, std::move(state));
  });
}

extern "C" JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_installLegacyBindingsNow(
    JNIEnv *env,
    jobject module,
    jlong runtimePointer) {
  if (runtimePointer == 0) {
    return;
  }
  auto state = std::make_shared<SKRNNativeVideo::AndroidModuleState>(env, module);
  auto *runtime = reinterpret_cast<facebook::jsi::Runtime *>(runtimePointer);
  installLegacyRuntime(*runtime, std::move(state));
}
