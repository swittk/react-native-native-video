#include "NativeVideoBindings.h"

#include <ReactCommon/CallInvoker.h>

#include "SKAndroidNativeVideoCPP.h"
#include "react-native-native-video.h"

using namespace facebook;

namespace SKRNNativeVideo {

void NativeVideoBindings::registerNatives() {
  javaClassLocal()->registerNatives({
      makeNativeMethod(
          "getBindingsInstaller", NativeVideoBindings::getBindingsInstaller),
  });
}

jni::local_ref<react::BindingsInstallerHolder::javaobject>
NativeVideoBindings::getBindingsInstaller(
    jni::alias_ref<NativeVideoBindings> module) {
  auto state = std::make_shared<AndroidModuleState>(
      jni::Environment::current(), module.get());
  return react::BindingsInstallerHolder::newObjectCxxArgs(
      [state = std::move(state)](
          jsi::Runtime &runtime,
          const std::shared_ptr<react::CallInvoker> &) {
        SKRNNativeVideo::install(
            runtime,
            [state](jsi::Runtime &, const std::string &path) {
              return std::make_shared<SKAndroidNativeVideoWrapper>(path, state);
            });
      });
}

} // namespace SKRNNativeVideo
