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
        [view showDisplayBuffer:wrapper->buffer transform:wrapper->transform];
//        [view showDisplayBuffer:wrapper->buffer orientation:wrapper->orientation];
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
    image = [image imageByApplyingTransform:transform];
//    size_t width = CVPixelBufferGetWidth(buf);
//    size_t height = CVPixelBufferGetHeight(buf);
    UIImage *uiImage = [UIImage imageWithCIImage:image];
    self.image = uiImage;
    CFRelease(buf);
}
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer orientation:(UIImageOrientation)orientation {
    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
    CFRetain(buf);
    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
//    size_t width = CVPixelBufferGetWidth(buf);
//    size_t height = CVPixelBufferGetHeight(buf);
    UIImage *uiImage = [UIImage imageWithCIImage:image scale:1.0 orientation:orientation];
    self.image = uiImage;
    CFRelease(buf);
}

-(void)setImage:(UIImage *)image {
    _image = image;
    imageView.image = image;
}
@end
