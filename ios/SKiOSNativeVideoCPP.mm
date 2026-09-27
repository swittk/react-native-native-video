#include "SKiOSNativeVideoCPP.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "iOSVideoUtils.h"

#import <CommonCrypto/CommonDigest.h>

using namespace facebook;

namespace SKRNNativeVideo {
namespace {

CGImagePropertyOrientation toCGImageOrientation(UIImageOrientation orientation) {
  switch (orientation) {
    case UIImageOrientationUp:
      return kCGImagePropertyOrientationUp;
    case UIImageOrientationDown:
      return kCGImagePropertyOrientationDown;
    case UIImageOrientationLeft:
      return kCGImagePropertyOrientationLeft;
    case UIImageOrientationRight:
      return kCGImagePropertyOrientationRight;
    case UIImageOrientationUpMirrored:
      return kCGImagePropertyOrientationUpMirrored;
    case UIImageOrientationDownMirrored:
      return kCGImagePropertyOrientationDownMirrored;
    case UIImageOrientationLeftMirrored:
      return kCGImagePropertyOrientationLeftMirrored;
    case UIImageOrientationRightMirrored:
      return kCGImagePropertyOrientationRightMirrored;
  }
}

NSURL *urlFromSource(const std::string &sourceUri) {
  NSString *source = [NSString stringWithUTF8String:sourceUri.c_str()];
  NSURL *url = [NSURL URLWithString:source];
  if (url != nil && url.scheme.length > 0) {
    return url.isFileURL ? url : nil;
  }
  return [NSURL fileURLWithPath:source];
}

NSArray<NSValue *> *sortedTimes(NSSet<NSValue *> *times) {
  return [[times allObjects] sortedArrayUsingComparator:^NSComparisonResult(
                    NSValue *left, NSValue *right) {
    return static_cast<NSComparisonResult>(
        CMTimeCompare(left.CMTimeValue, right.CMTimeValue));
  }];
}

double seconds(CMTime time) {
  const double value = CMTimeGetSeconds(time);
  return std::isfinite(value) ? value : 0;
}

std::string md5ForData(NSData *data) {
  unsigned char digest[CC_MD5_DIGEST_LENGTH];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  CC_MD5(data.bytes, static_cast<CC_LONG>(data.length), digest);
#pragma clang diagnostic pop

  char output[CC_MD5_DIGEST_LENGTH * 2 + 1];
  for (size_t index = 0; index < CC_MD5_DIGEST_LENGTH; ++index) {
    snprintf(output + index * 2, 3, "%02x", digest[index]);
  }
  output[CC_MD5_DIGEST_LENGTH * 2] = '\0';
  return output;
}

} // namespace

SKiOSNativeVideoWrapper::SKiOSNativeVideoWrapper(const std::string &sourceUri)
    : SKNativeVideoWrapper(sourceUri) {
  @autoreleasepool {
    NSURL *url = urlFromSource(sourceUri);
    if (url == nil) {
      return;
    }
    asset = [AVURLAsset URLAssetWithURL:url options:nil];
    if (!loadVideoTrack() || !buildFrameTimeMap() ||
        !createRandomAccessReader()) {
      close();
      return;
    }
    setValid(true);
  }
}

SKiOSNativeVideoWrapper::~SKiOSNativeVideoWrapper() {
  close();
}

bool SKiOSNativeVideoWrapper::loadVideoTrack() {
  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  AVURLAsset *loadingAsset = asset;
  __block NSArray<AVAssetTrack *> *tracks = nil;
  __block NSError *error = nil;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  [loadingAsset loadValuesAsynchronouslyForKeys:@[ @"tracks" ]
                       completionHandler:^{
                         AVKeyValueStatus status =
                             [loadingAsset statusOfValueForKey:@"tracks" error:&error];
                         if (status == AVKeyValueStatusLoaded) {
                           tracks = [loadingAsset tracksWithMediaType:AVMediaTypeVideo];
                         }
                         dispatch_semaphore_signal(semaphore);
                       }];
#pragma clang diagnostic pop
  const dispatch_time_t timeout =
      dispatch_time(DISPATCH_TIME_NOW, 15 * NSEC_PER_SEC);
  if (dispatch_semaphore_wait(semaphore, timeout) != 0) {
    lastError = [NSError
        errorWithDomain:@"SKRNNativeVideo"
                   code:408
               userInfo:@{
                 NSLocalizedDescriptionKey :
                     @"Timed out while loading the local video track"
               }];
    return false;
  }

  if (error != nil || tracks.count == 0) {
    lastError = error ?: [NSError
        errorWithDomain:@"SKRNNativeVideo"
                   code:404
               userInfo:@{NSLocalizedDescriptionKey : @"Asset has no video track"}];
    return false;
  }

  videoTrack = tracks.firstObject;

  // AVFoundation's modern typed async property API is Swift-only. Once the
  // track itself has been loaded asynchronously, Objective-C++ must still read
  // this presentation hint through the compatibility property.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  preferredTransform = videoTrack.preferredTransform;
#pragma clang diagnostic pop
  orientation = SKRNNVRotationValueToUIImageOrientation(
      SKRNNVCGAffineTransformGetRotation(preferredTransform));
  return true;
}

bool SKiOSNativeVideoWrapper::buildFrameTimeMap() {
  NSError *error = nil;
  AVAssetReader *fastReader = [AVAssetReader assetReaderWithAsset:asset error:&error];
  if (fastReader == nil || error != nil) {
    lastError = error;
    return false;
  }

  AVAssetReaderTrackOutput *fastOutput =
      [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack
                                       outputSettings:nil];
  if (![fastReader canAddOutput:fastOutput]) {
    return false;
  }
  [fastReader addOutput:fastOutput];
  if (![fastReader startReading]) {
    lastError = fastReader.error;
    return false;
  }

  NSMutableSet<NSValue *> *uniqueTimes = [NSMutableSet set];
  CMTime maximumEndTime = kCMTimeZero;
  CGSize encodedSize = CGSizeZero;

  for (CMSampleBufferRef buffer = [fastOutput copyNextSampleBuffer];
       buffer != nullptr;
       buffer = [fastOutput copyNextSampleBuffer]) {
    const CMTime presentationTime =
        CMSampleBufferGetOutputPresentationTimeStamp(buffer);
    const CMTime sampleDuration = CMSampleBufferGetOutputDuration(buffer);
    const size_t sampleSize = CMSampleBufferGetTotalSampleSize(buffer);

    if (CMTIME_IS_VALID(presentationTime) && sampleSize > 0) {
      [uniqueTimes addObject:[NSValue valueWithCMTime:presentationTime]];
      const CMTime endTime = CMTIME_IS_NUMERIC(sampleDuration)
          ? CMTimeAdd(presentationTime, sampleDuration)
          : presentationTime;
      if (CMTimeCompare(endTime, maximumEndTime) > 0) {
        maximumEndTime = endTime;
      }

      if (CGSizeEqualToSize(encodedSize, CGSizeZero)) {
        CMFormatDescriptionRef format =
            CMSampleBufferGetFormatDescription(buffer);
        if (format != nullptr &&
            CMFormatDescriptionGetMediaType(format) == kCMMediaType_Video) {
          encodedSize = CMVideoFormatDescriptionGetPresentationDimensions(
              static_cast<CMVideoFormatDescriptionRef>(format), true, true);
        }
      }
    }
    CFRelease(buffer);
  }

  frameTimeMap = sortedTimes(uniqueTimes);
  numFrames_ = static_cast<int>(frameTimeMap.count);
  duration_ = seconds(maximumEndTime);

  if (numFrames_ > 1) {
    const double first = seconds(frameTimeMap.firstObject.CMTimeValue);
    const double last = seconds(frameTimeMap.lastObject.CMTimeValue);
    if (last > first) {
      frameRate_ = static_cast<double>(numFrames_ - 1) / (last - first);
    }
  }
  if (duration_ <= 0 && frameRate_ > 0) {
    duration_ = static_cast<double>(numFrames_) / frameRate_;
  }

  const bool swapsAxes = orientation == UIImageOrientationLeft ||
      orientation == UIImageOrientationRight ||
      orientation == UIImageOrientationLeftMirrored ||
      orientation == UIImageOrientationRightMirrored;
  size_ = swapsAxes
      ? SKRNSize{encodedSize.height, encodedSize.width}
      : SKRNSize{encodedSize.width, encodedSize.height};

  return numFrames_ > 0;
}

bool SKiOSNativeVideoWrapper::createRandomAccessReader() {
  NSError *error = nil;
  reader = [AVAssetReader assetReaderWithAsset:asset error:&error];
  if (reader == nil || error != nil) {
    lastError = error;
    return false;
  }

  readerOutput = [[AVAssetReaderTrackOutput alloc]
      initWithTrack:videoTrack
     outputSettings:@{
       (__bridge NSString *)kCVPixelBufferPixelFormatTypeKey :
           @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
     }];
  readerOutput.supportsRandomAccess = YES;
  if (![reader canAddOutput:readerOutput]) {
    return false;
  }
  [reader addOutput:readerOutput];

  const CMTime firstTime = frameTimeMap.firstObject.CMTimeValue;
  reader.timeRange = CMTimeRangeMake(firstTime, frameDurationAtIndex(0));
  if (![reader startReading]) {
    lastError = reader.error;
    return false;
  }

  // Exhaust the initial range before resetForReadingTimeRanges is used.
  for (CMSampleBufferRef buffer = [readerOutput copyNextSampleBuffer];
       buffer != nullptr;
       buffer = [readerOutput copyNextSampleBuffer]) {
    CFRelease(buffer);
  }
  return true;
}

CMTime SKiOSNativeVideoWrapper::frameDurationAtIndex(int index) const {
  if (index >= 0 && index + 1 < numFrames_) {
    return CMTimeSubtract(
        frameTimeMap[index + 1].CMTimeValue,
        frameTimeMap[index].CMTimeValue);
  }
  const double fallback = frameRate_ > 0 ? 1.0 / frameRate_ : 1.0 / 30.0;
  return CMTimeMakeWithSeconds(fallback, 60000);
}

std::shared_ptr<SKNativeFrameWrapper>
SKiOSNativeVideoWrapper::getFrameAtTime(double time) {
  if (!valid_ || numFrames_ == 0 || time > duration_) {
    return nullptr;
  }
  return getFrameAtIndex(frameIndexAtTime(time));
}

double SKiOSNativeVideoWrapper::frameTimestampAtIndex(int index) const {
  if (index < 0 || index >= numFrames_) {
    return 0;
  }
  return seconds(frameTimeMap[index].CMTimeValue);
}

int SKiOSNativeVideoWrapper::frameIndexAtTime(double time) const {
  if (numFrames_ <= 0) {
    return -1;
  }
  int low = 0;
  int high = numFrames_;
  while (low < high) {
    const int middle = low + (high - low) / 2;
    if (frameTimestampAtIndex(middle) <= time) {
      low = middle + 1;
    } else {
      high = middle;
    }
  }
  return std::max(0, std::min(numFrames_ - 1, low - 1));
}

std::shared_ptr<SKNativeFrameWrapper>
SKiOSNativeVideoWrapper::getFrameAtIndex(int index) {
  if (!valid_ || index < 0 || index >= numFrames_) {
    return nullptr;
  }

  const CMTime frameTime = frameTimeMap[index].CMTimeValue;
  const CMTimeRange range =
      CMTimeRangeMake(frameTime, frameDurationAtIndex(index));
  [readerOutput resetForReadingTimeRanges:@[[NSValue valueWithCMTimeRange:range]]];

  CMSampleBufferRef buffer = [readerOutput copyNextSampleBuffer];
  if (buffer == nullptr) {
    return nullptr;
  }
  const CMTime decodedTimestamp =
      CMSampleBufferGetOutputPresentationTimeStamp(buffer);
  auto result = std::make_shared<SKiOSNativeFrameWrapper>(
      buffer,
      preferredTransform,
      orientation,
      index,
      CMTIME_IS_NUMERIC(decodedTimestamp)
          ? seconds(decodedTimestamp)
          : frameTimestampAtIndex(index));
  CFRelease(buffer);

  for (CMSampleBufferRef extra = [readerOutput copyNextSampleBuffer];
       extra != nullptr;
       extra = [readerOutput copyNextSampleBuffer]) {
    CFRelease(extra);
  }
  return result;
}

std::vector<std::shared_ptr<SKNativeFrameWrapper>>
SKiOSNativeVideoWrapper::getFramesAtIndex(int index, int requestedFrames) {
  std::vector<std::shared_ptr<SKNativeFrameWrapper>> result;
  if (!valid_ || index < 0 || index >= numFrames_ || requestedFrames <= 0) {
    return result;
  }

  const int count = std::min(requestedFrames, numFrames_ - index);
  const int exclusiveEnd = index + count;
  const CMTime start = frameTimeMap[index].CMTimeValue;
  const CMTime end = exclusiveEnd < numFrames_
      ? frameTimeMap[exclusiveEnd].CMTimeValue
      : CMTimeAdd(
            frameTimeMap[numFrames_ - 1].CMTimeValue,
            frameDurationAtIndex(numFrames_ - 1));
  [readerOutput resetForReadingTimeRanges:@[
    [NSValue valueWithCMTimeRange:CMTimeRangeFromTimeToTime(start, end)]
  ]];

  result.reserve(count);
  for (CMSampleBufferRef buffer = [readerOutput copyNextSampleBuffer];
       buffer != nullptr;
       buffer = [readerOutput copyNextSampleBuffer]) {
    if (CMSampleBufferIsValid(buffer) &&
        CMTIME_IS_VALID(CMSampleBufferGetOutputPresentationTimeStamp(buffer)) &&
        static_cast<int>(result.size()) < count) {
      const int frameIndex = index + static_cast<int>(result.size());
      result.push_back(std::make_shared<SKiOSNativeFrameWrapper>(
          buffer,
          preferredTransform,
          orientation,
          frameIndex,
          seconds(CMSampleBufferGetOutputPresentationTimeStamp(buffer))));
    }
    CFRelease(buffer);
  }
  return result;
}

void SKiOSNativeVideoWrapper::close() {
  if (reader != nil) {
    [reader cancelReading];
  }
  setValid(false);
  frameTimeMap = nil;
  readerOutput = nil;
  reader = nil;
  videoTrack = nil;
  asset = nil;
  lastError = nil;
}

SKiOSNativeFrameWrapper::SKiOSNativeFrameWrapper(
    CMSampleBufferRef sampleBuffer,
    CGAffineTransform frameTransform,
    UIImageOrientation frameOrientation,
    int frameIndex,
    double frameTimestamp)
    : SKNativeFrameWrapper(frameIndex, frameTimestamp),
      transform(frameTransform),
      orientation(frameOrientation) {
  if (sampleBuffer != nullptr) {
    CFRetain(sampleBuffer);
    buffer = sampleBuffer;
    setValid(true);
  }
}

SKiOSNativeFrameWrapper::~SKiOSNativeFrameWrapper() {
  close();
}

void *SKiOSNativeFrameWrapper::nativeBufferPointer() const {
  if (!valid_ || buffer == nullptr || !CMSampleBufferIsValid(buffer)) {
    return nullptr;
  }
  return static_cast<void *>(CMSampleBufferGetImageBuffer(buffer));
}

jsi::Value SKiOSNativeFrameWrapper::arrayBufferValue(jsi::Runtime &runtime) {
  const UInt32MallocatedPointerStruct raw =
      RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, orientation);
  if (raw.ptr == nullptr) {
    throw jsi::JSError(runtime, "Unable to convert the iOS frame to RGBA8");
  }
  std::unique_ptr<UInt32, decltype(&free)> rawBytes(raw.ptr, &free);

  jsi::Function constructor =
      runtime.global().getPropertyAsFunction(runtime, "ArrayBuffer");
  jsi::Object object = constructor
                           .callAsConstructor(
                               runtime, jsi::Value(static_cast<double>(raw.len)))
                           .getObject(runtime);
  jsi::ArrayBuffer arrayBuffer = object.getArrayBuffer(runtime);
  memcpy(arrayBuffer.data(runtime), rawBytes.get(), raw.len);
  return object;
}

SKRNSize SKiOSNativeFrameWrapper::size() const {
  if (hasSize_) {
    return size_;
  }
  if (buffer == nullptr || !CMSampleBufferIsValid(buffer)) {
    return {0, 0};
  }

  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(buffer);
  const double width = static_cast<double>(CVPixelBufferGetWidth(imageBuffer));
  const double height = static_cast<double>(CVPixelBufferGetHeight(imageBuffer));
  const bool swapsAxes = orientation == UIImageOrientationLeft ||
      orientation == UIImageOrientationRight ||
      orientation == UIImageOrientationLeftMirrored ||
      orientation == UIImageOrientationRightMirrored;
  size_ = swapsAxes ? SKRNSize{height, width} : SKRNSize{width, height};
  hasSize_ = true;
  return size_;
}

size_t SKiOSNativeFrameWrapper::bytesPerRow() const {
  return static_cast<size_t>(size().width) * 4;
}

std::string SKiOSNativeFrameWrapper::base64(const std::string &requestedFormat) {
  if (buffer == nullptr || !CMSampleBufferIsValid(buffer)) {
    return {};
  }
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(buffer);
  if (imageBuffer == nullptr) {
    return {};
  }
  CIImage *image = [CIImage imageWithCVPixelBuffer:imageBuffer];
  image = [image imageByApplyingOrientation:toCGImageOrientation(orientation)];
  UIImage *uiImage = [UIImage imageWithCIImage:image];

  NSString *format = [NSString stringWithUTF8String:requestedFormat.c_str()];
  NSData *data = ([format caseInsensitiveCompare:@"jpg"] == NSOrderedSame ||
                  [format caseInsensitiveCompare:@"jpeg"] == NSOrderedSame)
      ? UIImageJPEGRepresentation(uiImage, 1.0)
      : UIImagePNGRepresentation(uiImage);
  return data == nil
      ? std::string()
      : std::string([[data base64EncodedStringWithOptions:0] UTF8String]);
}

std::string SKiOSNativeFrameWrapper::md5() {
  NSData *data = RawRGBA32NSDataFromCMSampleBuffer(buffer, orientation);
  return data == nil ? std::string() : md5ForData(data);
}

void SKiOSNativeFrameWrapper::close() {
  if (buffer != nullptr) {
    CFRelease(buffer);
    buffer = nullptr;
  }
  setValid(false);
}

double SKRNNVCGAffineTransformGetRotation(CGAffineTransform transform) {
  return atan2(transform.b, transform.a);
}

UIImageOrientation SKRNNVRotationValueToUIImageOrientation(double rotation) {
  const double quarterTurns = std::round(rotation / (M_PI / 2.0));
  int normalized = static_cast<int>(quarterTurns) % 4;
  if (normalized < 0) {
    normalized += 4;
  }
  switch (normalized) {
    case 1:
      return UIImageOrientationRight;
    case 2:
      return UIImageOrientationDown;
    case 3:
      return UIImageOrientationLeft;
    default:
      return UIImageOrientationUp;
  }
}

} // namespace SKRNNativeVideo
