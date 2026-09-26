#import <AVFoundation/AVFoundation.h>
#import <React/RCTViewManager.h>

@interface SKRNNativeFrameView : UIView
@property (nonatomic, readonly, nullable) UIImage *image;
@end

@interface SKRNNativeFrameViewManager : RCTViewManager
@end
