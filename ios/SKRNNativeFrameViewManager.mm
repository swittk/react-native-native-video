#import "SKRNNativeFrameViewManager.h"

#import "SKiOSNativeVideoCPP.h"
#import "react-native-native-video.h"

#include <memory>
#include <utility>

using namespace SKRNNativeVideo;

namespace {

CGImagePropertyOrientation CGOrientationForUIImageOrientation(
    UIImageOrientation orientation) {
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
  return kCGImagePropertyOrientationUp;
}

} // namespace

@interface SKRNNativeFrameView () {
  UIImageView *_imageView;
  std::shared_ptr<SKiOSNativeFrameWrapper> _nativeFrame;
}

- (void)setNativeFrame:(std::shared_ptr<SKiOSNativeFrameWrapper>)frame;
- (void)setNativeResizeMode:(NSString *_Nullable)resizeMode;

@end

@implementation SKRNNativeFrameViewManager

RCT_EXPORT_MODULE(SKRNNativeFrameView)

- (SKRNNativeFrameView *)view
{
  return [[SKRNNativeFrameView alloc] initWithFrame:CGRectZero];
}

RCT_CUSTOM_VIEW_PROPERTY(frameId, NSString *, SKRNNativeFrameView)
{
  if (![json isKindOfClass:[NSString class]]) {
    [view setNativeFrame:nullptr];
    return;
  }

  auto frame = std::dynamic_pointer_cast<SKiOSNativeFrameWrapper>(
      resolveNativeFrame(std::string([(NSString *)json UTF8String])));
  [view setNativeFrame:std::move(frame)];
}

RCT_CUSTOM_VIEW_PROPERTY(resizeMode, NSString *, SKRNNativeFrameView)
{
  [view setNativeResizeMode:
      [json isKindOfClass:[NSString class]] ? (NSString *)json : nil];
}

@end

@implementation SKRNNativeFrameView

- (instancetype)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];
  if (self != nil) {
    [self commonInit];
  }
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self != nil) {
    [self commonInit];
  }
  return self;
}

- (void)commonInit
{
  _imageView = [[UIImageView alloc] initWithFrame:self.bounds];
  _imageView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  _imageView.contentMode = UIViewContentModeScaleAspectFit;
  [self addSubview:_imageView];
}

- (UIImage *)image
{
  return _imageView.image;
}

- (void)setNativeResizeMode:(NSString *)resizeMode
{
  if ([resizeMode isEqualToString:@"cover"]) {
    _imageView.contentMode = UIViewContentModeScaleAspectFill;
  } else if ([resizeMode isEqualToString:@"stretch"]) {
    _imageView.contentMode = UIViewContentModeScaleToFill;
  } else {
    _imageView.contentMode = UIViewContentModeScaleAspectFit;
  }
}

- (void)setNativeFrame:(std::shared_ptr<SKiOSNativeFrameWrapper>)frame
{
  _nativeFrame = std::move(frame);
  if (!_nativeFrame || !_nativeFrame->isValid() ||
      _nativeFrame->buffer == nullptr) {
    _imageView.image = nil;
    return;
  }

  CVImageBufferRef imageBuffer =
      CMSampleBufferGetImageBuffer(_nativeFrame->buffer);
  if (imageBuffer == nullptr) {
    _imageView.image = nil;
    return;
  }

  CIImage *image = [CIImage imageWithCVPixelBuffer:imageBuffer];
  image = [image imageByApplyingOrientation:
      CGOrientationForUIImageOrientation(_nativeFrame->orientation)];
  CIContext *context = [CIContext contextWithOptions:nil];
  CGImageRef rendered = [context createCGImage:image fromRect:image.extent];
  if (rendered == nullptr) {
    _imageView.image = nil;
    return;
  }
  _imageView.image = [UIImage imageWithCGImage:rendered];
  CGImageRelease(rendered);
}

@end
