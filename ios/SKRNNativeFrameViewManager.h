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
@end

@interface SKRNNativeFrameViewManager : RCTViewManager

@end
