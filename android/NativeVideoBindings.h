#ifndef NATIVE_VIDEO_BINDINGS_H
#define NATIVE_VIDEO_BINDINGS_H

#include <ReactCommon/BindingsInstallerHolder.h>
#include <fbjni/fbjni.h>

namespace SKRNNativeVideo {

class NativeVideoBindings : public facebook::jni::JavaClass<NativeVideoBindings> {
 public:
  static constexpr const char *kJavaDescriptor =
      "Lcom/reactnativenativevideo/NativeVideoModule;";

  static void registerNatives();

 private:
  static facebook::jni::local_ref<
      facebook::react::BindingsInstallerHolder::javaobject>
  getBindingsInstaller(facebook::jni::alias_ref<NativeVideoBindings> module);
};

} // namespace SKRNNativeVideo

#endif
