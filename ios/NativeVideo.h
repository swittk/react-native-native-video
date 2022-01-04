#import <React/RCTBridgeModule.h>
#ifdef __cplusplus

#import "react-native-native-video.h"

#endif

@interface NativeVideo : NSObject <RCTBridgeModule>
@property (nonatomic, assign) BOOL setBridgeOnMainQueue;
@end
