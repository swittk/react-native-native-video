//
//  SKiOSNativeVideoCPP.hpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//
#import <AVFoundation/AVFoundation.h>
#ifdef __cplusplus
#import "react-native-native-video.h"
#endif

#ifndef SKiOSNativeVideoCPP_hpp
#define SKiOSNativeVideoCPP_hpp

@interface SKRNNVRetainChecker : NSObject
@end

namespace SKRNNativeVideo {
double SKRNNVCGAffineTransformGetRotation(CGAffineTransform transform);
UIImageOrientation SKRNNVRotationValueToUIImageOrientation(double rotation);

class SKiOSNativeFrameWrapper : public SKNativeFrameWrapper {
    bool _hasSize = false;
    SKRNSize _size;
public:
    CMSampleBufferRef buffer;
    CGAffineTransform transform;
    UIImageOrientation orientation;
    /** This buffer is CFRetained +1 by the wrapper. You are supposed to call CFRelease() on any other dependencies on this buffer yourself.*/
    SKiOSNativeFrameWrapper(CMSampleBufferRef buf, CGAffineTransform transform, UIImageOrientation orientation);
    ~SKiOSNativeFrameWrapper();
    /** This is potentially for casting the correct type  (should return "iOS" for iOS and "Android" for Android)*/
    virtual std::string platform() { return "iOS"; };
    // This should free/close native resources
    virtual void close();
    // Supposed to return ArrayBuffer
    virtual facebook::jsi::Value arrayBufferValue(facebook::jsi::Runtime &runtime);
    virtual SKRNSize size();
    virtual std::string base64(std::string format);
    virtual std::string md5();
};

class SKiOSNativeVideoWrapper : public SKNativeVideoWrapper {
    int _numFrames;
    NSArray <NSValue *>*frameTimeMap;
    UIImageOrientation orientation = UIImageOrientationUp;
    bool _hasSize = false;
    SKRNSize _size;
    
public:
    NSError *_lastError;
    AVAsset *asset;
    AVAssetReader *reader;
    AVAssetTrack *videoTrack;
    AVAssetReaderTrackOutput *readerOutput;
    SKRNNVRetainChecker *checker;
    SKiOSNativeVideoWrapper(std::string sourceUri);
    ~SKiOSNativeVideoWrapper();
    virtual void close();
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index);
    virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(int index, int numFrames);
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time);
    virtual int numFrames() {return _numFrames;};
    virtual double frameRate();
    virtual SKRNSize size();
    virtual double duration();
private:
    void initialReadAsset();
};

};

#endif /* SKiOSNativeVideoCPP_hpp */
