//
//  SKRNNativeFrameViewManager.m
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#import "SKRNNativeFrameViewManager.h"
#import "SKiOSNativeVideoCPP.h"
#import "react-native-native-video.h"

@interface SKRNNativeFrameView() {
    UIImageView *imageView;
}
@property (readonly) UIImageView *imageView;
@end

using namespace SKRNNativeVideo;
@implementation SKRNNativeFrameViewManager
RCT_EXPORT_MODULE(SKRNNativeFrameView)

CGImagePropertyOrientation SKRNNVCGImagePropertyOrientationForUIImageOrientation(UIImageOrientation uiOrientation);

- (SKRNNativeFrameView *)view
{
  return [[SKRNNativeFrameView alloc] init];
}

RCT_CUSTOM_VIEW_PROPERTY(resizeMode, NSString *, SKRNNativeFrameView) {
    if(![json isKindOfClass:[NSString class]]) {
        NSLog(@"got weird resizeMode");
        return;
    }
    NSString *mode = (NSString *)json;
    if([mode isEqualToString:@"contain"]) {
        view.imageView.contentMode = UIViewContentModeScaleAspectFit;
    }
    else if([mode isEqualToString:@"cover"]) {
        view.imageView.contentMode = UIViewContentModeScaleAspectFill;
    }
    else if([mode isEqualToString:@"stretch"]) {
        view.imageView.contentMode = UIViewContentModeScaleToFill;
    }
}

RCT_CUSTOM_VIEW_PROPERTY(frameData, id, SKRNNativeFrameView) {
    if(!json) return;
//    NSLog(@"got frame %@", json);
    // Doing this because I couldn't find any resources on how to pass JSI stuff to native code :/
    if(json[@"nativePtrStr"]) {
        NSString *str = json[@"nativePtrStr"];
        std::string cppstr = std::string([str UTF8String]);
        
        // If it looks stupid but it works, then it ain't stupid ;)
        void *ptr = SKRNNativeVideo::StringToPointer(cppstr);
        if(ptr == nullptr) {
            NSLog(@"sadly pointer goes to null");
            return;
        }
        SKiOSNativeFrameWrapper *wrapper = (SKiOSNativeFrameWrapper *)ptr;
//        [view showDisplayBuffer:wrapper->buffer transform:wrapper->transform];
        [view showDisplayBuffer:wrapper->buffer orientation:wrapper->orientation];
    }
}


@end

@implementation SKRNNativeFrameView {
}
@synthesize image = _image;
@synthesize imageView;
-(id)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if(!self) return nil;
    [self commonInit];
    return self;
}
-(id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if(!self) return nil;
    [self commonInit];
    return self;
}
-(void)commonInit {
    imageView = [[UIImageView alloc] initWithFrame:self.bounds];
    imageView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    imageView.contentMode = UIViewContentModeScaleAspectFit;
    [self addSubview:imageView];
}
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer {
    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
    CFRetain(buf);
    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
//    size_t width = CVPixelBufferGetWidth(buf);
//    size_t height = CVPixelBufferGetHeight(buf);
    UIImage *uiImage = [UIImage imageWithCIImage:image];
    self.image = uiImage;
    CFRelease(buf);
}
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer transform:(CGAffineTransform)transform {
    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
    CFRetain(buf);
    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
    NSLog(@"transform was %@", NSStringFromCGAffineTransform(transform));
//    if(!CGAffineTransformIsIdentity(transform)) {
        // Manually invert the transform for iOS
    
    CGSize imageSize = CVImageBufferGetDisplaySize(buf);
    // CoreImage coordinate system origin is at the bottom left corner
    // and UIKit is at the top left corner. So we need to translate
    // features positions before drawing them to screen. In order to do
    // so we make an affine transform
    CGAffineTransform ciTransform = CGAffineTransformMakeScale(1, -1);
//    ciTransform = CGAffineTransformTranslate(ciTransform,
//                                        0, -imageSize.height);
    
    
    //    transform.b = -transform.b;
    //    transform.c = -transform.c;
    image = [image imageByApplyingTransform:transform];
//    }
    UIImage *uiImage = [UIImage imageWithCIImage:image];
    self.image = uiImage;
    CFRelease(buf);
}
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer orientation:(UIImageOrientation)orientation {
    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
    if(!buf) {
        NSLog(@"SKRNNV : unable to get CVImageBufferRef from CMSampleBufferRef");
    }
    CFRetain(buf);
    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
    // As for why we're using imageByApplyingOrientation instead of just creating [UIImage imageWithCIImage:size:orientation:], the reason is that the orientation property somehow is disregarded by the UIImageView with that method.
    image = [image imageByApplyingOrientation:SKRNNVCGImagePropertyOrientationForUIImageOrientation(orientation)];
//    size_t width = CVPixelBufferGetWidth(buf);
//    size_t height = CVPixelBufferGetHeight(buf);
    UIImage *uiImage = [UIImage imageWithCIImage:image];
//    NSLog(@"applying orientation %d got %d", orientation, uiImage.imageOrientation);
    self.image = uiImage;
    CFRelease(buf);
}


-(void)setImage:(UIImage *)image {
    _image = image;
    imageView.image = image;
}
@end


CGImagePropertyOrientation SKRNNVCGImagePropertyOrientationForUIImageOrientation(UIImageOrientation uiOrientation) {
    switch (uiOrientation) {
        case UIImageOrientationUp: return kCGImagePropertyOrientationUp;
        case UIImageOrientationDown: return kCGImagePropertyOrientationDown;
        case UIImageOrientationLeft: return kCGImagePropertyOrientationLeft;
        case UIImageOrientationRight: return kCGImagePropertyOrientationRight;
        case UIImageOrientationUpMirrored: return kCGImagePropertyOrientationUpMirrored;
        case UIImageOrientationDownMirrored: return kCGImagePropertyOrientationDownMirrored;
        case UIImageOrientationLeftMirrored: return kCGImagePropertyOrientationLeftMirrored;
        case UIImageOrientationRightMirrored: return kCGImagePropertyOrientationRightMirrored;
    }
}
UIImageOrientation SKRNNVUIImageOrientationForCGImagePropertyOrientation(CGImagePropertyOrientation cgOrientation) {
    switch (cgOrientation) {
        case kCGImagePropertyOrientationUp: return UIImageOrientationUp;
        case kCGImagePropertyOrientationDown: return UIImageOrientationDown;
        case kCGImagePropertyOrientationLeft: return UIImageOrientationLeft;
        case kCGImagePropertyOrientationRight: return UIImageOrientationRight;
        case kCGImagePropertyOrientationUpMirrored: return UIImageOrientationUpMirrored;
        case kCGImagePropertyOrientationDownMirrored: return UIImageOrientationDownMirrored;
        case kCGImagePropertyOrientationLeftMirrored: return UIImageOrientationLeftMirrored;
        case kCGImagePropertyOrientationRightMirrored: return UIImageOrientationRightMirrored;
    }
}
