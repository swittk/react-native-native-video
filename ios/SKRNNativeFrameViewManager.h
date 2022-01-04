//
//  SKRNNativeFrameViewManager.h
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#import <React/RCTViewManager.h>
#import <AVFoundation/AVFoundation.h>

@interface SKRNNativeFrameView : UIView
@property (readonly) UIImage *image;
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer;
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer transform:(CGAffineTransform)transform;
-(void)showDisplayBuffer:(CMSampleBufferRef)buffer orientation:(UIImageOrientation)orientation;
@end

@interface SKRNNativeFrameViewManager : RCTViewManager

@end
