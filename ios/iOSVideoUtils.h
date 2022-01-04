//
//  iOSVideoUtils.hpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>

#ifndef iOSVideoUtils_hpp
#define iOSVideoUtils_hpp

namespace SKRNNativeVideo {
typedef struct Float32MallocatedPointerStruct {
    Float32 *ptr;
    size_t len;
} Float32MallocatedPointerStruct;
}

CFDictionaryRef SKRN_GetCVPixelBufferAttributesForGlCompatibility();

// Taken from https://github.com/google/mediapipe/blob/1faeaae7e504657f44544989f9547fc69652e487/mediapipe/objc/util.cc#L183
vImage_Error SKRN_vImageConvertCVPixelBuffers(CVPixelBufferRef src,
                                         CVPixelBufferRef dst);

vImageConverterRef SKRN_vImageConverterForCVPixelFormats(OSType src_pixel_format,
                                                    OSType dst_pixel_format,
                                                    vImage_Error* error);

CGColorSpaceRef SKRN_CreateConversionCGColorSpaceForPixelFormat(OSType pixel_format);

vImage_CGImageFormat SKRN_vImageFormatForCVPixelFormat(OSType pixel_format);

CVPixelBufferRef SKRN_convertPixelBufferToPixelFormat(CVPixelBufferRef input, OSType pixelFormat);

SKRNNativeVideo::Float32MallocatedPointerStruct rawDataFromCMSampleBuffer(CMSampleBufferRef buffer);

#endif /* iOSVideoUtils_hpp */
