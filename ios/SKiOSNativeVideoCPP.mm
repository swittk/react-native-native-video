//
//  SKiOSNativeVideoCPP.cpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#include "SKiOSNativeVideoCPP.h"
#include "iOSVideoUtils.h"
#import <CommonCrypto/CommonDigest.h>

using namespace SKRNNativeVideo;
using namespace facebook;
#pragma mark - SKiOSNativeVideoWrapper
static int clampInt(int val, int min, int max);
static CMTime cmTimeFromValue(NSValue *value);

static NSArray <NSValue *>*sortedCMTimeArrayFromCMTimeSet(NSSet <NSValue *>*inSet);
static CGImagePropertyOrientation SKRNNVCGImagePropertyOrientationForUIImageOrientation(UIImageOrientation uiOrientation) {
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

SKiOSNativeVideoWrapper::SKiOSNativeVideoWrapper(std::string sourceUri) : SKNativeVideoWrapper(sourceUri)
{
    // See here https://developer.apple.com/forums/thread/42751 :: Resource for reading CMSampleBufferRef frames from video
    NSString *path = [NSString stringWithUTF8String:sourceUri.c_str()];
    if([path hasPrefix:@"file:///"]) {
        path = [path substringFromIndex:7];
    }
    
    asset = [AVAsset assetWithURL:[NSURL fileURLWithPath:path]];
    
    NSArray <AVAssetTrack *>*videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];
    if(![videoTracks count]) {
        _lastError = [NSError errorWithDomain:@"SKNativeVideo" code:404 userInfo:@{NSLocalizedDescriptionKey:@"The asset does not have any video tracks"}];
        NSLog(@"Found no video tracks");
        return;
    }
    videoTrack = videoTracks[0];
    // Fast-read the asset to populate the timestamps for each frame
    initialReadAsset();
    
    
    NSError *assetReaderError;
    reader = [AVAssetReader assetReaderWithAsset:asset error:&assetReaderError];
    if(assetReaderError) {
        _lastError = assetReaderError;
        NSLog(@"Error reading asset, %@", _lastError);
        return;
    }
    NSLog(@"created reader");
    // Need to specify outputSettings, or CMSampleBufferGetImageBuffer will return NULL (since the frames are not decompressed)
    @try {
        readerOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:@{
            (__bridge NSString *)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
        }];
    } @catch(NSError *e) {
        NSLog(@"caught error with reading, %@", e);
        return;
    }
    //    reader.timeRange = kCMTimeRangeZero;
    // Make sure we support random access otherwise we won't be able to seek!
    readerOutput.supportsRandomAccess = YES;
    [reader addOutput:readerOutput];
    reader.timeRange = CMTimeRangeMake(CMTimeMake(0, 10000), CMTimeMake(1, 10000));
    BOOL ok = [reader startReading];
    if(!ok) {
        _lastError = reader.error;
        NSLog(@"Failed to read asset, error %@", _lastError);
        return;
    }
    // Make sure readerOutput reads till null so we can reset safely
    CMSampleBufferRef trash = [readerOutput copyNextSampleBuffer];
    while(trash != NULL) {
        CMSampleBufferInvalidate(trash);
        CFRelease(trash);
        trash = [readerOutput copyNextSampleBuffer];
    }
    //    checker = [SKRNNVRetainChecker new];
    setValid(true);
}
SKiOSNativeVideoWrapper::~SKiOSNativeVideoWrapper() {
    close();
}

// This skims through the asset and get the frame time maps
void SKiOSNativeVideoWrapper::initialReadAsset() {
    double rotation = SKRNNVCGAffineTransformGetRotation(videoTrack.preferredTransform);
    orientation = SKRNNVRotationValueToUIImageOrientation(rotation);
    NSLog(@"got orientation %d", orientation);
    NSError *assetReaderError;
    AVAssetReader *fastReader = [AVAssetReader assetReaderWithAsset:asset error:&assetReaderError];
    if(assetReaderError) {
        _lastError = assetReaderError;
        NSLog(@"Error reading asset, %@", _lastError);
        return;
    }
    AVAssetReaderTrackOutput *fastOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:nil];
    //
    [fastReader addOutput:fastOutput];
    BOOL ok = [fastReader startReading];
    if(!ok) {
        _lastError = [NSError errorWithDomain:@"SKNativeVideo" code:404 userInfo:@{NSLocalizedDescriptionKey:@"The asset reader failed to read"}];
        NSLog(@"Failed to read asset");
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
    CMSampleBufferRef buffer = [fastOutput copyNextSampleBuffer];
    NSMutableSet <NSValue *>*existingValues = [NSMutableSet new];
    //    CMTime mostRecentTime = CMTimeMake(0, 60);
    while(buffer != NULL) {
        // Get size of buffer because sometimes it's zero and we don't want that
        // It appears that sometimes size of CMSampleBuffer is zero
        // This occurs at the start of the video (frameIndex 0), and at the last frame of the video
        size_t size = CMSampleBufferGetSampleSize(buffer, 0);
        CMTime frameTime = CMSampleBufferGetOutputPresentationTimeStamp(buffer);
        //        if(CMTimeCompare(mostRecentTime, frameTime) > 0) {
        //            NSLog(@"previous time more than nowTime for %@", [NSValue valueWithCMTime:frameTime]);
        //        }
        //        mostRecentTime = frameTime;
        // CMTime seems to be roughly arranged (not always in order), for example, in a 60FPS iPhone video
        // the previous time more than nowTime dialogue occurs roughly 16 times / second
        NSValue *frameTimeValue = [NSValue valueWithCMTime:frameTime];
        if(CMTIME_IS_INVALID(frameTime) || size == 0) {
            // Do not process if invalid or has no size (generally occurs at either start or end of video)
        }
        else {
            if([existingValues containsObject:frameTimeValue]) {
                // If already has CMTime for this frameIndex, skip
                // Usually get `Already has object for CMTime: {0/600 = 0.00}` (or {INVALID} sometimes)
                NSLog(@"Already has object for time %@ currentCount %d", frameTimeValue, frameIndex);
            }
            else {
                [existingValues addObject:frameTimeValue];
                frameIndex++;
            }
        }
        CMSampleBufferInvalidate(buffer);
        CFRelease(buffer);
        // Next loop
        buffer = [fastOutput copyNextSampleBuffer];
    }
    frameTimeMap = sortedCMTimeArrayFromCMTimeSet(existingValues);
    // Set the numFrames
    _numFrames = frameIndex;
    // This works fine
    //    NSLog(@"got numframes and frameTimeMap %d, %@", _numFrames, frameTimeMap);
}

// Inspired by this https://stackoverflow.com/a/9046695/4469172
NSArray <NSValue *>*sortedCMTimeArrayFromCMTimeSet(NSSet <NSValue *>*inSet) {
    NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:nil ascending:YES comparator:^NSComparisonResult(NSValue *obj1, NSValue *obj2) {
        return (NSComparisonResult)CMTimeCompare(obj1.CMTimeValue, obj2.CMTimeValue);
    }];
    NSArray *ret = [inSet sortedArrayUsingDescriptors:@[sortDescriptor]];
    return ret;
}

std::shared_ptr<SKNativeFrameWrapper> SKiOSNativeVideoWrapper::getFrameAtTime(double time) {
    int approximateFrame = time / videoTrack.nominalFrameRate;
    return getFrameAtIndex(approximateFrame);
}
std::shared_ptr<SKNativeFrameWrapper> SKiOSNativeVideoWrapper::getFrameAtIndex(int frameIdx) {
    if(frameIdx >= frameTimeMap.count || frameIdx < 0) {
        return std::shared_ptr<SKNativeFrameWrapper>(nullptr);
    }
    NSValue *value = [frameTimeMap objectAtIndex:frameIdx];
    //    NSLog(@"got value for frameIdx %d, value %@", frameIdx, value);
    CMTime cmTime = cmTimeFromValue(value);
    [readerOutput resetForReadingTimeRanges:@[[NSValue valueWithCMTimeRange:CMTimeRangeMake(cmTime, CMTimeMakeWithSeconds(1/frameRate(), videoTrack.naturalTimeScale))]]];
    CMSampleBufferRef buf = [readerOutput copyNextSampleBuffer];
    if(buf == NULL) {
        return std::shared_ptr<SKNativeFrameWrapper>(nullptr);
    }
    std::shared_ptr<SKiOSNativeFrameWrapper> ret = std::make_shared<SKiOSNativeFrameWrapper>(buf, videoTrack.preferredTransform, orientation);
    CFRelease(buf);
    
    int extraCount = 0;
    CMSampleBufferRef trash = [readerOutput copyNextSampleBuffer];
    while(trash != NULL) {
        CMSampleBufferInvalidate(trash);
        CFRelease(trash);
        extraCount++;
        trash = [readerOutput copyNextSampleBuffer];
    }
    //    NSLog(@"got frame at index %d with extraCount %d", frameIdx, extraCount);
    return std::move(ret);
}

std::vector<std::shared_ptr<SKNativeFrameWrapper>> SKiOSNativeVideoWrapper::getFramesAtIndex(int index, int numFrames) {
    index = clampInt(index, 0, _numFrames - 1);
    // Need to use exclusiveTo (gone over by 1 frame, because it seems resetForReadingTimeRanges excludes the `to` time);
    int exclusiveToIndex = index + numFrames;
    NSValue *value = [frameTimeMap objectAtIndex:index];
    CMTime fromTime = cmTimeFromValue(value);
    
    CMTime toTime;
    if(exclusiveToIndex > _numFrames - 1) {
        //        toTime = kCMTimePositiveInfinity;
        // If the `exclusive` index does not exist, then we simply move it to the last one and go up by a bit
        NSValue *toValue = [frameTimeMap lastObject];
        toTime = cmTimeFromValue(toValue);
        // Add 0.5 of the timescale to the value in order to `go over` the last frame's time by a bit so it gets fetched too.
        toTime.timescale = toTime.timescale * 2;
        toTime.value = toTime.value * 2 + 1;
    }
    else {
        NSValue *toValue = [frameTimeMap objectAtIndex:exclusiveToIndex];
        toTime = cmTimeFromValue(toValue);
    }
    [readerOutput resetForReadingTimeRanges:@[[NSValue valueWithCMTimeRange:CMTimeRangeFromTimeToTime(fromTime, toTime)]]];
    CMSampleBufferRef buf = [readerOutput copyNextSampleBuffer];
    std::vector<std::shared_ptr<SKNativeFrameWrapper>> ret;
    while(buf != NULL) {
        // CMSampleBufferGetSampleSize doesn't work for decoded frames (will return 0 in size always).
        CMTime frameTime = CMSampleBufferGetOutputPresentationTimeStamp(buf);
        
        if(!CMSampleBufferIsValid(buf)) {
            // Do nothing
        }
        else if(CMTIME_IS_INVALID(frameTime)) {
            // Do nothing
        }
        else {
            ret.push_back(std::make_shared<SKiOSNativeFrameWrapper>(buf, videoTrack.preferredTransform, orientation));
        }
        // Just release, don't do an oopsie and accidentally invalidate it like last time
        CFRelease(buf);
        buf = [readerOutput copyNextSampleBuffer];
    }
    return ret;
}
double SKiOSNativeVideoWrapper::frameRate() {
    return videoTrack.nominalFrameRate;
}

SKRNSize SKiOSNativeVideoWrapper::size() {
    if(_hasSize) {
        return _size;
    }
    AVAssetTrack *track = videoTrack;
    CGSize size = CGSizeApplyAffineTransform(track.naturalSize, track.preferredTransform);
    _size = (SKRNSize){ .width = size.width, .height = size.height };
    _hasSize = true;
    return _size;
}

double SKiOSNativeVideoWrapper::duration() {
    CMTime val = videoTrack.asset.duration;
    return (double)val.value / (double)val.timescale;
}

void SKiOSNativeVideoWrapper::close() {
    NSLog(@"video close() called");
    //    CFBridgingRelease((__bridge void *)_lastError);
    //    CFBridgingRelease((__bridge void *)frameTimeMap);
    //    CFBridgingRelease((__bridge void *)asset);
    //    CFBridgingRelease((__bridge void *)reader);
    //    CFBridgingRelease((__bridge void *)videoTrack);
    //    CFBridgingRelease((__bridge void *)readerOutput);
    //    NSLog(@"released all");
    _lastError = nil;
    frameTimeMap = nil;
    asset = nil;
    reader = nil;
    videoTrack = nil;
    readerOutput = nil;
    checker = nil;
    setValid(false);
}
//SKiOSNativeFrameWrapper SKiOSNativeVideoWrapper::getNativeFrame() {
//    return SKiOSNativeFrameWrapper()
//}


#pragma mark - SKiOSNativeFrameWrapper

SKiOSNativeFrameWrapper::SKiOSNativeFrameWrapper(CMSampleBufferRef buf, CGAffineTransform vtransform, UIImageOrientation vorientation) : SKNativeFrameWrapper(), transform(vtransform), orientation(vorientation) {
    CFRetain(buf);
    buffer = buf;
    setValid(true);
}
facebook::jsi::Value SKiOSNativeFrameWrapper::arrayBufferValue(facebook::jsi::Runtime &runtime) {
    // TODO: allow specifying output formats
    //    jsi::ArrayBuffer
//    Float32MallocatedPointerStruct ret = RawFloat32RGBScaledDataFromDataFromCMSampleBuffer(buffer);
//    if(ret.ptr == NULL) {
//        NSLog(@"rawDataPtr is NULL");
//        return facebook::jsi::Value::undefined();
//    }
    UInt32MallocatedPointerStruct ret = RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, orientation);
    if(ret.ptr == NULL) {
        NSLog(@"rawDataPtr is NULL");
        return facebook::jsi::Value::undefined();
    }
    // Create ArrayBuffer
    jsi::Function arrayBufferCtor = runtime.global().getPropertyAsFunction(runtime, "ArrayBuffer");
    size_t totalBytes = ret.len;
    jsi::Object o = arrayBufferCtor.callAsConstructor(runtime, jsi::Value((int)totalBytes)).getObject(runtime);
    jsi::ArrayBuffer buf = o.getArrayBuffer(runtime);
    memcpy(buf.data(runtime), ret.ptr, totalBytes);
    free(ret.ptr);
    return std::move(o);
}

SKRNSize SKiOSNativeFrameWrapper::size() {
    if(!CMSampleBufferIsValid(buffer)) {
        NSLog(@"Sample buffer is invalid");
        return (SKRNSize){0, 0};
    }
    if(_hasSize) {
        return _size;
    }
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(buffer);
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    size_t width = CVPixelBufferGetWidth(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    //    NSLog(@"Got frame size %zu, %zu", width, height);
    _size = (SKRNSize){.width = (double)width, .height = (double)height};
    _hasSize = true;
    return _size;
}

std::string SKiOSNativeFrameWrapper::base64(std::string format) {
    if(!buffer) return std::string();
    if(!format.size()) {
        format = "png";
    }
    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
    CFRetain(buf);
    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
    image = [image imageByApplyingOrientation:SKRNNVCGImagePropertyOrientationForUIImageOrientation(orientation)];
    //    size_t width = CVPixelBufferGetWidth(buf);
    //    size_t height = CVPixelBufferGetHeight(buf);
    UIImage *uiImage = [UIImage imageWithCIImage:image];
    
    NSString *str;
    if(format == "png") {
        str = [UIImagePNGRepresentation(uiImage) base64EncodedStringWithOptions:0];
    }
    else if(format == "jpg" || format == "jpeg") {
        str = [UIImageJPEGRepresentation(uiImage, 1.0) base64EncodedStringWithOptions:0];
    }
    else {
        // Return PNG
        str = [UIImagePNGRepresentation(uiImage) base64EncodedStringWithOptions:0];
    }
    CFRelease(buf);
    return std::string([str UTF8String]);
}

// Adapted from https://gist.github.com/fcandalija/1359520
static std::string md5ForNSData(NSData *data) {
    // Create byte array of unsigned chars
    unsigned char md5Buffer[CC_MD5_DIGEST_LENGTH];
    // Create 16 byte MD5 hash value, store in buffer
    CC_MD5(data.bytes, (unsigned int)data.length, md5Buffer);
    // Convert unsigned char buffer to NSString of hex values
        NSMutableString *output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
        for(int i = 0; i < CC_MD5_DIGEST_LENGTH; i++)
            [output appendFormat:@"%02x",md5Buffer[i]];
    return std::string([output cStringUsingEncoding:NSUTF8StringEncoding]);
}

std::string SKiOSNativeFrameWrapper::md5() {
    if(!buffer) return std::string();
    NSData *data = RawRGBA32NSDataFromCMSampleBuffer(buffer, orientation);
    std::string ret = md5ForNSData(data);
//    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(buffer);
//    CFRetain(buf);
//    CIImage *image = [CIImage imageWithCVPixelBuffer:buf];
//    image = [image imageByApplyingOrientation:SKRNNVCGImagePropertyOrientationForUIImageOrientation(orientation)];
//    //    size_t width = CVPixelBufferGetWidth(buf);
//    //    size_t height = CVPixelBufferGetHeight(buf);
//    UIImage *uiImage = [UIImage imageWithCIImage:image];
//    NSData *pngData = UIImageJPEGRepresentation(uiImage, 1);
//    std::string ret = md5ForNSData(pngData);
//    CFRelease(buf);
    return ret;
}

void SKiOSNativeFrameWrapper::close() {
    if(buffer != NULL) {
        CMSampleBufferInvalidate(buffer);
        CFRelease(buffer);
        buffer = NULL;
        setValid(false);
    }
}
SKiOSNativeFrameWrapper::~SKiOSNativeFrameWrapper() {
    close();
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

double SKRNNativeVideo::SKRNNVCGAffineTransformGetRotation(CGAffineTransform transform) {
    // using a flipped transform since it's iOS (should be transform.b, transform.a in most other systems)
    /*
     | a  b  0 |  | 0    1   0 |
     | c  d  0 |  | -1   0   0 |
     | tx ty 1 |  | 1080 0   1 |
     */
    return atan2(transform.b, transform.a);
}

UIImageOrientation SKRNNativeVideo::SKRNNVRotationValueToUIImageOrientation(double rotation) {
    // UIImageRotations are clockwise
    NSLog(@"rotation was %f", rotation);
    if(rotation >= 0) {
        int switchVal = (int)(rotation/M_PI_4);
        NSLog(@"switchVal %d", switchVal);
        switch(switchVal) {
            default:
            case 0: return UIImageOrientationUp; // 0-45°
            case 1: // 45-90°
            case 2: return UIImageOrientationRight; // 90-135°
            case 3: // 135-180°
            case 4: return UIImageOrientationDown; // 180-225°
            case 5: // 225-270°
            case 6: return UIImageOrientationLeft; // 270-315°
            case 7: // 270-315°
            case 8: return UIImageOrientationUp; // 315-360°
        }
    }
    else {
        // Rotation < 0
        int switchVal = (int)(-rotation/M_PI_4);
        NSLog(@"switchVal %d", switchVal);
        switch(switchVal) {
            default:
            case 0: return UIImageOrientationUp; // 0-45°
            case 1: // 45-90°
            case 2: return UIImageOrientationLeft; // 90-135°
            case 3: // 135-180°
            case 4: return UIImageOrientationDown; // 180-225°
            case 5: // 225-270°
            case 6: return UIImageOrientationRight; // 270-315°
            case 7: // 270-315°
            case 8: return UIImageOrientationUp; // 315-360°
        }
    }
}

@implementation SKRNNVRetainChecker

-(void)dealloc {
    NSLog(@"dealloc called");
}

@end
