#ifndef SK_ANDROID_NATIVE_VIDEO_CPP_H
#define SK_ANDROID_NATIVE_VIDEO_CPP_H

#include <jni.h>
#include <android/hardware_buffer.h>

#include <memory>
#include <mutex>
#include <string>

#include "react-native-native-video.h"

namespace SKRNNativeVideo {

struct AndroidModuleState {
  AndroidModuleState(JNIEnv *env, jobject nativeVideoModule);
  ~AndroidModuleState();

  JavaVM *jvm = nullptr;
  jobject module = nullptr;
};

void initializeJavaBindings(JNIEnv *env);

class SKAndroidNativeFrameWrapper final : public SKNativeFrameWrapper {
 public:
  SKAndroidNativeFrameWrapper(
      JavaVM *jvm,
      JNIEnv *env,
      jobject javaFrame,
      int index,
      double timestamp);
  ~SKAndroidNativeFrameWrapper() override;

  std::string platform() const override { return "Android"; }
  std::string nativeBufferType() const override;
  void *nativeBufferPointer() const override;
  void close() override;
  facebook::jsi::Value arrayBufferValue(
      facebook::jsi::Runtime &runtime) override;
  SKRNSize size() const override;
  size_t bytesPerRow() const override;
  std::string base64(const std::string &format) override;

  jobject bitmap(JNIEnv *env) const;

 private:
  mutable std::mutex frameMutex_;
  JavaVM *jvm_ = nullptr;
  jobject javaFrame_ = nullptr;
  AHardwareBuffer *hardwareBuffer_ = nullptr;
};

class SKAndroidNativeVideoWrapper final : public SKNativeVideoWrapper {
 public:
  SKAndroidNativeVideoWrapper(
      const std::string &sourceUri,
      std::shared_ptr<AndroidModuleState> moduleState);
  ~SKAndroidNativeVideoWrapper() override;

  void close() override;
  std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index) override;
  std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(
      int index,
      int numFrames) override;
  std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time) override;
  int numFrames() const override;
  double frameRate() const override;
  SKRNSize size() const override;
  double duration() const override;
  double frameTimestampAtIndex(int index) const override;
  int frameIndexAtTime(double time) const override;

 private:
  std::shared_ptr<AndroidModuleState> moduleState_;
  jobject javaVideoWrapper_ = nullptr;
};

} // namespace SKRNNativeVideo

#endif
