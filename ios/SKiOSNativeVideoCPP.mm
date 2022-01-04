//
//  SKiOSNativeVideoCPP.cpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#include "SKiOSNativeVideoCPP.h"

using namespace SKRNNativeVideo;
#pragma mark - SKiOSNativeVideoWrapper
static int clampInt(int val, int min, int max);
static CMTime cmTimeFromValue(NSValue *value);

SKiOSNativeVideoWrapper::SKiOSNativeVideoWrapper(facebook::jsi::Runtime &runtime, std::string sourceUri) : SKNativeVideoWrapper(runtime, sourceUri)
 {
     // See here https://developer.apple.com/forums/thread/42751 :: Resource for reading CMSampleBufferRef frames from video
     NSString *path = [NSString stringWithUTF8String:sourceUri.c_str()];
     if([path hasPrefix:@"file:///"]) {
         path = [path substringFromIndex:7];
     }

    asset = [AVAsset assetWithURL:[NSURL fileURLWithPath:path]];
     NSError *assetReaderError;
     reader = [AVAssetReader assetReaderWithAsset:asset error:&assetReaderError];
     if(assetReaderError) {
         _lastError = assetReaderError;
         return;
     }
     NSArray <AVAssetTrack *>*videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];
     if(![videoTracks count]) {
         _lastError = [NSError errorWithDomain:@"SKNativeVideo" code:404 userInfo:@{NSLocalizedDescriptionKey:@"The asset does not have any video tracks"}];
         return;
     }
     videoTrack = videoTracks[0];
     readerOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:nil];
     //
     readerOutput.supportsRandomAccess = YES;
     [reader addOutput:readerOutput];
     BOOL ok = [reader startReading];
     if(!ok) {
         _lastError = [NSError errorWithDomain:@"SKNativeVideo" code:404 userInfo:@{NSLocalizedDescriptionKey:@"The asset reader failed to read"}];
         return;
     }
     // Funny little thing about sample cursors; they aren't that known lol
     // Also doesn't work on iOS :(
     // See https://developer.apple.com/forums/thread/119613
//     if([videoTrack canProvideSampleCursors]) { }
//     CMTimeScale timeScale = videoTrack.naturalTimeScale;
//     videoTrack.nominalFrameRate;
     int frameIndex = 0;
     // Generate map of CMTime for people
     NSMutableDictionary <NSNumber *, NSValue *>*frameTimeMapObj = [NSMutableDictionary new];
     CMSampleBufferRef buffer = [readerOutput copyNextSampleBuffer];
     while(buffer != NULL) {
        CMTime frameTime = CMSampleBufferGetOutputPresentationTimeStamp(buffer);
        [frameTimeMapObj setObject:[NSValue valueWithCMTime:frameTime] forKey:@(frameIndex)];
         frameIndex++;
        // Next loop
        buffer = [readerOutput copyNextSampleBuffer];
     }
     
     frameTimeMap = frameTimeMapObj;
     // Set the numFrames
     _numFrames = frameIndex;
}

std::shared_ptr<SKNativeFrameWrapper> SKiOSNativeVideoWrapper::getFrameAtTime(double time) {
    int approximateFrame = time / videoTrack.nominalFrameRate;
    return getFrameAtIndex(approximateFrame);
}
std::shared_ptr<SKNativeFrameWrapper> SKiOSNativeVideoWrapper::getFrameAtIndex(int frameIdx) {
    NSValue *value = [frameTimeMap objectForKey:@(frameIdx)];
    CMTime cmTime = cmTimeFromValue(value);
    [readerOutput resetForReadingTimeRanges:@[[NSValue valueWithCMTimeRange:CMTimeRangeMake(cmTime, kCMTimeZero)]]];
    CMSampleBufferRef buf = [readerOutput copyNextSampleBuffer];
    std::shared_ptr<SKiOSNativeFrameWrapper> ret = std::make_shared<SKiOSNativeFrameWrapper>(buf);
    CFRelease(buf);
    return std::move(ret);
}

std::vector<std::shared_ptr<SKNativeFrameWrapper>> SKiOSNativeVideoWrapper::getFramesAtIndex(int index, int numFrames) {
    index = clampInt(index, 0, _numFrames);
    int toIndex = index + numFrames;
    toIndex = clampInt(toIndex, 0, _numFrames);
    NSValue *value = [frameTimeMap objectForKey:@(index)];
    NSValue *toValue = [frameTimeMap objectForKey:@(toIndex)];
    CMTime fromTime = cmTimeFromValue(value);
    CMTime toTime = cmTimeFromValue(toValue);
    [readerOutput resetForReadingTimeRanges:@[[NSValue valueWithCMTimeRange:CMTimeRangeFromTimeToTime(fromTime, toTime)]]];
    CMSampleBufferRef buf = [readerOutput copyNextSampleBuffer];
    std::vector<std::shared_ptr<SKNativeFrameWrapper>> ret;
    while(buf != NULL) {
        ret.push_back(std::make_shared<SKiOSNativeFrameWrapper>(buf));
        CFRelease(buf);
        buf = [readerOutput copyNextSampleBuffer];
    }
    return ret;
}

double SKiOSNativeVideoWrapper::frameRate() {
    return videoTrack.nominalFrameRate;
}

void SKiOSNativeVideoWrapper::close() {
    _lastError = nil;
    asset = nil;
    reader = nil;
    videoTrack = nil;
    readerOutput = nil;
}
//SKiOSNativeFrameWrapper SKiOSNativeVideoWrapper::getNativeFrame() {
//    return SKiOSNativeFrameWrapper()
//}


#pragma mark - SKiOSNativeFrameWrapper

SKiOSNativeFrameWrapper::SKiOSNativeFrameWrapper(CMSampleBufferRef buf) {
    CFRetain(buf);
    buffer = buf;
    setValid(true);
}
void SKiOSNativeFrameWrapper::close() {
    if(buffer) {
        CMSampleBufferInvalidate(buffer);
        CFRelease(buffer);
        buffer = NULL;
        setValid(false);
    }
}
SKiOSNativeFrameWrapper::~SKiOSNativeFrameWrapper() {
    if(buffer != NULL) {
        close();
    }
}

int clampInt(int val, int min, int max) {
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

CMTime cmTimeFromValue(NSValue *value) {
    if(value) { return  [value CMTimeValue]; }
    return CMTimeMake(0, 0);
}

