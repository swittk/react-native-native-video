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

namespace SKRNNativeVideo {
class SKiOSNativeFrameWrapper : public SKNativeFrameWrapper {
public:
    CMSampleBufferRef buffer;
    /** This buffer is CFRetained +1 by the wrapper. You are supposed to call CFRelease() on any other dependencies on this buffer yourself.*/
    SKiOSNativeFrameWrapper(CMSampleBufferRef buf);
    ~SKiOSNativeFrameWrapper();
    void close();
};

class SKiOSNativeVideoWrapper : public SKNativeVideoWrapper {
    int _numFrames;
    NSDictionary <NSNumber *, NSValue *>*frameTimeMap;
public:
    NSError *_lastError;
    AVAsset *asset;
    AVAssetReader *reader;
    AVAssetTrack *videoTrack;
    AVAssetReaderTrackOutput *readerOutput;
    SKiOSNativeVideoWrapper(facebook::jsi::Runtime &runtime, std::string sourceUri);
    virtual void close();
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index);
    virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(int index, int numFrames);
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time);
    virtual double frameRate();
};

}

#endif /* SKiOSNativeVideoCPP_hpp */