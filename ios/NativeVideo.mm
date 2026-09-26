#import "NativeVideo.h"

#import "SKiOSNativeVideoCPP.h"
#import "react-native-native-video.h"

#ifdef RCT_NEW_ARCH_ENABLED
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>
#else
#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#endif

using namespace facebook;
using namespace SKRNNativeVideo;

#ifdef RCT_NEW_ARCH_ENABLED
@interface NativeVideo () <RCTTurboModuleWithJSIBindings>
@end
#else
@interface NativeVideo ()
@property (nonatomic, weak) RCTBridge *bridge;
@property (nonatomic, assign) jsi::Runtime *installedRuntime;
@property (nonatomic, assign) BOOL invalidated;
@end
#endif

@implementation NativeVideo

RCT_EXPORT_MODULE(NativeVideo)

+ (BOOL)requiresMainQueueSetup
{
  return YES;
}

#ifdef RCT_NEW_ARCH_ENABLED

- (NSNumber *)multiply:(double)a b:(double)b
{
  return @(a * b);
}

- (std::shared_ptr<react::TurboModule>)getTurboModule:
    (const react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<react::NativeVideoSpecJSI>(params);
}

- (void)installJSIBindingsWithRuntime:(jsi::Runtime &)runtime
                          callInvoker:(const std::shared_ptr<react::CallInvoker> &)callInvoker
{
  (void)callInvoker;
  SKRNNativeVideo::install(
      runtime,
      [](jsi::Runtime &, const std::string &path) {
        return std::make_shared<SKiOSNativeVideoWrapper>(path);
      });
}

#else

@synthesize bridge = _bridge;

RCT_EXPORT_METHOD(multiply:(nonnull NSNumber *)a
                  withB:(nonnull NSNumber *)b
                  withResolver:(RCTPromiseResolveBlock)resolve
                  withReject:(RCTPromiseRejectBlock)reject)
{
  (void)reject;
  resolve(@([a doubleValue] * [b doubleValue]));
}

- (void)setBridge:(RCTBridge *)bridge
{
  _bridge = bridge;
  _invalidated = NO;
  _installedRuntime = nullptr;
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(javaScriptDidLoad:)
             name:RCTJavaScriptDidLoadNotification
           object:bridge];
  [self installLegacyJSIBindingsIfReady];
}

- (void)javaScriptDidLoad:(NSNotification *)notification
{
  (void)notification;
  [self installLegacyJSIBindingsIfReady];
}

- (void)installLegacyJSIBindingsIfReady
{
  if (_invalidated) {
    return;
  }
  RCTBridge *bridge = _bridge;
  if (bridge == nil) {
    return;
  }

  RCTBridge *runtimeBridge = [bridge isKindOfClass:[RCTCxxBridge class]]
      ? bridge
      : bridge.batchedBridge;
  if (![runtimeBridge isKindOfClass:[RCTCxxBridge class]]) {
    return;
  }
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)runtimeBridge;
  auto *runtime = static_cast<jsi::Runtime *>(cxxBridge.runtime);
  if (runtime == nullptr || runtime == _installedRuntime) {
    return;
  }

  SKRNNativeVideo::install(
      *runtime,
      [](jsi::Runtime &, const std::string &path) {
        return std::make_shared<SKiOSNativeVideoWrapper>(path);
      });
  _installedRuntime = runtime;
}

- (void)invalidate
{
  _invalidated = YES;
  _installedRuntime = nullptr;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _bridge = nil;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#endif

@end
