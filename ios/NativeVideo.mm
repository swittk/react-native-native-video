#import "NativeVideo.h"
#import "react-native-native-video.h"
#import <React/RCTUtils.h>
#import <React/RCTBridge+Private.h>
#import <jsi/jsi.h>
#import <ReactCommon/CallInvoker.h>
#import "SKiOSNativeVideoCPP.h"

using namespace SKRNNativeVideo;

//SKRNNativeVideo::SKiOSNativeFrameWrapper

@implementation NativeVideo
@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

// Example method for C++
// See the implementation of the example module in the `cpp` folder
RCT_EXPORT_METHOD(multiply:(nonnull NSNumber*)a withB:(nonnull NSNumber*)b
                  withResolver:(RCTPromiseResolveBlock)resolve
                  withReject:(RCTPromiseRejectBlock)reject)
{
    NSNumber *result = @(multiply([a floatValue], [b floatValue]));
    
    resolve(result);
}


+ (BOOL)requiresMainQueueSetup {
    return YES;
}



- (void)setBridge:(RCTBridge *)bridge {
    _bridge = bridge;
    _setBridgeOnMainQueue = RCTIsMainQueue();
    [self installLibrary];
}

-(void)installLibrary {
//    self.bridge.reactInstance;
    RCTCxxBridge *cxxBridge = (RCTCxxBridge *)self.bridge;
    if (!cxxBridge.runtime) {
        
        /**
         * This is a workaround to install library
         * as soon as runtime becomes available and is
         * not recommended. If you see random crashes in iOS
         * global.xxx not found etc. use this.
         */
        
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.001 * NSEC_PER_SEC),
                       dispatch_get_main_queue(), ^{
            /**
             When refreshing the app while debugging, the setBridge
             method is called too soon. The runtime is not ready yet
             quite often. We need to install library as soon as runtime
             becomes available.
             */
            [self installLibrary];
            
        });
        return;
    }
    facebook::jsi::Runtime *runtime = (facebook::jsi::Runtime *)cxxBridge.runtime;
    install(*runtime, [](facebook::jsi::Runtime& runtime, std::string path) -> std::shared_ptr<SKNativeVideoWrapper> {
        std::shared_ptr<SKNativeVideoWrapper>ret =  std::make_shared<SKiOSNativeVideoWrapper>(runtime, path);
        return ret;
    });
}

- (void)invalidate {
    RCTCxxBridge *cxxBridge = (RCTCxxBridge *)self.bridge;
    facebook::jsi::Runtime *runtime = (facebook::jsi::Runtime *)cxxBridge.runtime;
    cleanup(*runtime);
}

@end
