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

typedef struct UInt32MallocatedPointerStruct {
    UInt32 *ptr;
    size_t len;
} UInt32MallocatedPointerStruct;

typedef struct UInt8MallocatedPointerStruct {
    UInt8 *ptr;
    size_t len;
} UInt8MallocatedPointerStruct;

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

// This is the core method
SKRNNativeVideo::UInt32MallocatedPointerStruct RawRGBA32DataFromCMSampleBufferAndOrientation(CMSampleBufferRef buffer, UIImageOrientation orientation);
SKRNNativeVideo::UInt32MallocatedPointerStruct RawRGBA32DataFromCMSampleBuffer(CMSampleBufferRef buffer);
SKRNNativeVideo::UInt8MallocatedPointerStruct RawBGR24DataFromCMSampleBuffer(CMSampleBufferRef buffer);

SKRNNativeVideo::UInt8MallocatedPointerStruct RawBGR24DataFromCMSampleBufferAndOrientation
(
 CMSampleBufferRef buffer, UIImageOrientation orientation
 );
SKRNNativeVideo::UInt8MallocatedPointerStruct RawRGB24DataFromCMSampleBufferAndOrientation
(
 CMSampleBufferRef buffer, UIImageOrientation orientation
 );


NSData *RawRGBA32NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation);
NSData *RawBGR24NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation);
NSData *RawRGB24NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation);

SKRNNativeVideo::Float32MallocatedPointerStruct RawFloat32RGBAScaledDataFromDataFromCMSampleBuffer(CMSampleBufferRef buffer);
NSData *RawFloat32RGBAScaledNSDataFromCMSampleBuffer(CMSampleBufferRef buffer);

SKRNNativeVideo::Float32MallocatedPointerStruct RawFloat32RGBScaledDataFromDataFromCMSampleBuffer(CMSampleBufferRef buffer);

CMSampleBufferRef CreateCMSampleBufferFromImage(UIImage *image, UIImageOrientation orientation);
#endif /* iOSVideoUtils_hpp */
