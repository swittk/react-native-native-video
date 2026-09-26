#include <jni.h>

#include <memory>
#include <string>

#include "SKAndroidNativeVideoCPP.h"
#include "react-native-native-video.h"

extern "C" JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_NativeVideoModule_installLegacyBindings(
    JNIEnv *env,
    jobject module,
    jlong runtimePointer) {
  if (runtimePointer == 0) {
    return;
  }

  auto state = std::make_shared<SKRNNativeVideo::AndroidModuleState>(env, module);
  auto *runtime = reinterpret_cast<facebook::jsi::Runtime *>(runtimePointer);
  SKRNNativeVideo::install(
      *runtime,
      [state = std::move(state)](
          facebook::jsi::Runtime &,
          const std::string &path) {
        return std::make_shared<SKRNNativeVideo::SKAndroidNativeVideoWrapper>(
            path, state);
      });
}
