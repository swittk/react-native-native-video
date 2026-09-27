#ifndef SK_IOS_NATIVE_VIDEO_CPP_H
#define SK_IOS_NATIVE_VIDEO_CPP_H

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

#include "react-native-native-video.h"

namespace SKRNNativeVideo {

double SKRNNVCGAffineTransformGetRotation(CGAffineTransform transform);
UIImageOrientation SKRNNVRotationValueToUIImageOrientation(double rotation);

class SKiOSNativeFrameWrapper final : public SKNativeFrameWrapper {
 public:
  CMSampleBufferRef buffer = nullptr;
  const CGAffineTransform transform;
  const UIImageOrientation orientation;

  SKiOSNativeFrameWrapper(
      CMSampleBufferRef buffer,
      CGAffineTransform transform,
      UIImageOrientation orientation,
      int index,
      double timestamp);
  ~SKiOSNativeFrameWrapper() override;

  std::string platform() const override { return "iOS"; }
  std::string nativeBufferType() const override { return "cvPixelBuffer"; }
  void *nativeBufferPointer() const override;
  void *retainNativeBufferPointer() const override;
  void releaseNativeBufferPointer(void *buffer) const override;
  void close() override;
  facebook::jsi::Value arrayBufferValue(
      facebook::jsi::Runtime &runtime) override;
  SKRNSize size() const override;
  size_t bytesPerRow() const override;
  std::string base64(const std::string &format) override;
  std::string md5() override;

 private:
  mutable bool hasSize_ = false;
  mutable SKRNSize size_{0, 0};
};

class SKiOSNativeVideoWrapper final : public SKNativeVideoWrapper {
 public:
  explicit SKiOSNativeVideoWrapper(const std::string &sourceUri);
  ~SKiOSNativeVideoWrapper() override;

  void close() override;
  std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index) override;
  std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(
      int index,
      int numFrames) override;
  std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time) override;
  int numFrames() const override { return numFrames_; }
  double frameRate() const override { return frameRate_; }
  SKRNSize size() const override { return size_; }
  double duration() const override { return duration_; }
  double frameTimestampAtIndex(int index) const override;
  int frameIndexAtTime(double time) const override;

 private:
  bool loadVideoTrack();
  bool buildFrameTimeMap();
  bool createRandomAccessReader();
  CMTime frameDurationAtIndex(int index) const;

  int numFrames_ = 0;
  double frameRate_ = 0;
  double duration_ = 0;
  SKRNSize size_{0, 0};
  NSArray<NSValue *> *frameTimeMap = nil;
  UIImageOrientation orientation = UIImageOrientationUp;
  CGAffineTransform preferredTransform = CGAffineTransformIdentity;

  NSError *lastError = nil;
  AVURLAsset *asset = nil;
  AVAssetReader *reader = nil;
  AVAssetTrack *videoTrack = nil;
  AVAssetReaderTrackOutput *readerOutput = nil;
};

} // namespace SKRNNativeVideo

#endif
