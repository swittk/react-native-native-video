#import <Foundation/Foundation.h>
#import <React/RCTBridgeModule.h>

#ifdef RCT_NEW_ARCH_ENABLED
#import <RNNativeVideoSpec/RNNativeVideoSpec.h>

@interface NativeVideo : NSObject <NativeVideoSpec>
#else
@interface NativeVideo : NSObject <RCTBridgeModule>
#endif

@end
