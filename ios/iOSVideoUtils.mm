//
//  iOSVideoUtils.cpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#include "iOSVideoUtils.h"
using namespace SKRNNativeVideo;

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

static void CVPixelBufferReleaseSimpleFreeMallocedBytesCallback(void *releaseRefCon, const void *baseAddress) {
    free((void *)baseAddress);
}



inline vImage_Buffer vImageForCVPixelBuffer(CVPixelBufferRef pixel_buffer) {
  return {.data = CVPixelBufferGetBaseAddress(pixel_buffer),
          .width = CVPixelBufferGetWidth(pixel_buffer),
          .height = CVPixelBufferGetHeight(pixel_buffer),
          .rowBytes = CVPixelBufferGetBytesPerRow(pixel_buffer)};
}


template <typename T, size_t N>
auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
#define ABSL_ARRAYSIZE(array) \
  (sizeof(ArraySizeHelper(array)))

CFDictionaryRef SKRN_GetCVPixelBufferAttributesForGlCompatibility() {
  static CFDictionaryRef attrs = NULL;
  if (!attrs) {
    CFDictionaryRef empty_dict = CFDictionaryCreate(
        kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    // To ensure compatibility with CVOpenGLESTextureCache, these attributes
    // should be present. However, on simulator this IOSurface attribute
    // actually causes CVOpenGLESTextureCache to fail. b/144850076
    const void* keys[] = {
#if !TARGET_IPHONE_SIMULATOR
      kCVPixelBufferIOSurfacePropertiesKey,
#endif  // !TARGET_IPHONE_SIMULATOR

#if TARGET_OS_OSX
      kCVPixelFormatOpenGLCompatibility,
#else
      kCVPixelFormatOpenGLESCompatibility,
#endif  // TARGET_OS_OSX
    };

    const void* values[] = {
#if !TARGET_IPHONE_SIMULATOR
      empty_dict,
#endif  // !TARGET_IPHONE_SIMULATOR
      kCFBooleanTrue
    };

    attrs = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, ABSL_ARRAYSIZE(values),
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFRelease(empty_dict);
  }
  return attrs;
}

vImage_Error SKRN_vImageConvertCVPixelBuffers(CVPixelBufferRef src,
                                         CVPixelBufferRef dst) {
  //  CGColorSpaceRef srgb_color_space =
  //  CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  vImage_Error error;
  vImageConverterRef converter = SKRN_vImageConverterForCVPixelFormats(
      CVPixelBufferGetPixelFormatType(src),
      CVPixelBufferGetPixelFormatType(dst), &error);
  if (!converter) {
    return error;
  }

  int src_buffer_count = vImageConverter_GetNumberOfSourceBuffers(converter);
  int dst_buffer_count =
      vImageConverter_GetNumberOfDestinationBuffers(converter);
  vImage_Buffer buffers[8];
  if (src_buffer_count + dst_buffer_count > ABSL_ARRAYSIZE(buffers)) {
    vImageConverter_Release(converter);
    return kvImageMemoryAllocationError;
  }
  vImage_Buffer* src_bufs = buffers;
  vImage_Buffer* dst_bufs = buffers + src_buffer_count;

  // vImageBuffer_InitForCopyToCVPixelBuffer can be used only if the converter
  // was created by vImageConverter_CreateForCGToCVImageFormat.
  // vImageBuffer_InitForCopyFromCVPixelBuffer can be used only if the converter
  // was created by vImageConverter_CreateForCVToCGImageFormat.
  // There does not seem to be a way to ask the converter for its type; however,
  // it is documented that all multi-planar formats are CV formats, so we use
  // these calls when there are multiple buffers.

  if (src_buffer_count > 1) {
    error = vImageBuffer_InitForCopyFromCVPixelBuffer(
        src_bufs, converter, src,
        kvImageNoAllocate | kvImagePrintDiagnosticsToConsole);
    if (error != kvImageNoError) {
      vImageConverter_Release(converter);
      return error;
    }
  } else {
    *src_bufs = vImageForCVPixelBuffer(src);
  }

  if (dst_buffer_count > 1) {
    error = vImageBuffer_InitForCopyToCVPixelBuffer(
        dst_bufs, converter, dst,
        kvImageNoAllocate | kvImagePrintDiagnosticsToConsole);
    if (error != kvImageNoError) {
      vImageConverter_Release(converter);
      return error;
    }
  } else {
    *dst_bufs = vImageForCVPixelBuffer(dst);
  }

  error = vImageConvert_AnyToAny(converter, src_bufs, dst_bufs, NULL,
                                 kvImageNoFlags);
  vImageConverter_Release(converter);
  return error;
}


vImageConverterRef SKRN_vImageConverterForCVPixelFormats(OSType src_pixel_format,
                                                    OSType dst_pixel_format,
                                                    vImage_Error* error) {
  static CGFloat default_background[3] = {1.0, 1.0, 1.0};
  vImageConverterRef converter = NULL;

  vImage_CGImageFormat src_cg_format =
    SKRN_vImageFormatForCVPixelFormat(src_pixel_format);
  vImage_CGImageFormat dst_cg_format =
    SKRN_vImageFormatForCVPixelFormat(dst_pixel_format);

  // Use CV format functions if available (introduced in iOS 8).
  // Weak-linked symbols are NULL when not available.
  if (&vImageConverter_CreateForCGToCVImageFormat != NULL) {
    // Strangely, there is no function to convert between two
    // vImageCVImageFormat, so one side has to use a vImage_CGImageFormat
    // that we have to find ourselves.
    if (src_cg_format.bitsPerComponent > 0) {
      // We can handle source using a CGImageFormat.
      // TODO: check the final alpha hint parameter
      CGColorSpaceRef cv_color_space =
        SKRN_CreateConversionCGColorSpaceForPixelFormat(dst_pixel_format);
      vImageCVImageFormatRef dst_cv_format = vImageCVImageFormat_Create(
          dst_pixel_format, kvImage_ARGBToYpCbCrMatrix_ITU_R_709_2,
          kCVImageBufferChromaLocation_Center, cv_color_space, 1);
      CGColorSpaceRelease(cv_color_space);

      converter = vImageConverter_CreateForCGToCVImageFormat(
          &src_cg_format, dst_cv_format, default_background,
          kvImagePrintDiagnosticsToConsole, error);
      vImageCVImageFormat_Release(dst_cv_format);
    } else if (dst_cg_format.bitsPerComponent > 0) {
      // We can use a CGImageFormat for the destination.
      CGColorSpaceRef cv_color_space =
        SKRN_CreateConversionCGColorSpaceForPixelFormat(src_pixel_format);
      vImageCVImageFormatRef src_cv_format = vImageCVImageFormat_Create(
          src_pixel_format, kvImage_ARGBToYpCbCrMatrix_ITU_R_709_2,
          kCVImageBufferChromaLocation_Center, cv_color_space, 1);
      CGColorSpaceRelease(cv_color_space);

      converter = vImageConverter_CreateForCVToCGImageFormat(
          src_cv_format, &dst_cg_format, default_background,
          kvImagePrintDiagnosticsToConsole, error);
      vImageCVImageFormat_Release(src_cv_format);
    }
  }

  if (!converter) {
    // Try a CG to CG conversion.
    if (src_cg_format.bitsPerComponent > 0 &&
        dst_cg_format.bitsPerComponent > 0) {
      converter = vImageConverter_CreateWithCGImageFormat(
          &src_cg_format, &dst_cg_format, default_background, kvImageNoFlags,
          error);
    }
  }

  CGColorSpaceRelease(src_cg_format.colorSpace);
  CGColorSpaceRelease(dst_cg_format.colorSpace);
  return converter;
}


CGColorSpaceRef SKRN_CreateConversionCGColorSpaceForPixelFormat(
    OSType pixel_format) {
  // According to vImage documentation, YUV formats require the RGB colorspace
  // in which the RGB conversion should be interpreted. sRGB is suggested.
  // We cannot just pass sRGB all the time, though, since it breaks with
  // monochrome.
  switch (pixel_format) {
    case kCVPixelFormatType_422YpCbCr8:
    case kCVPixelFormatType_4444YpCbCrA8:
    case kCVPixelFormatType_4444YpCbCrA8R:
    case kCVPixelFormatType_4444AYpCbCr8:
    case kCVPixelFormatType_4444AYpCbCr16:
    case kCVPixelFormatType_444YpCbCr8:
    case kCVPixelFormatType_422YpCbCr16:
    case kCVPixelFormatType_422YpCbCr10:
    case kCVPixelFormatType_444YpCbCr10:
    case kCVPixelFormatType_420YpCbCr8Planar:
    case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
    case kCVPixelFormatType_422YpCbCr_4A_8BiPlanar:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
    case kCVPixelFormatType_422YpCbCr8_yuvs:
    case kCVPixelFormatType_422YpCbCr8FullRange:
      return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    default:
      return NULL;
  }
}


vImage_CGImageFormat SKRN_vImageFormatForCVPixelFormat(OSType pixel_format) {
  switch (pixel_format) {
    case kCVPixelFormatType_OneComponent8:
      return {
          .bitsPerComponent = 8,
          .bitsPerPixel = 8,
          .colorSpace = CGColorSpaceCreateDeviceGray(),
          .bitmapInfo = kCGImageAlphaNone | kCGBitmapByteOrderDefault,
      };

    case kCVPixelFormatType_32BGRA:
      return {
          .bitsPerComponent = 8,
          .bitsPerPixel = 32,
          .colorSpace = NULL,
          .bitmapInfo = kCGImageAlphaFirst | kCGBitmapByteOrder32Little,
      };

    case kCVPixelFormatType_32RGBA:
      return {
          .bitsPerComponent = 8,
          .bitsPerPixel = 32,
          .colorSpace = NULL,
          .bitmapInfo = kCGImageAlphaLast | kCGBitmapByteOrderDefault,
      };

    default:
      return {};
  }
}


//Adapted from https://github.com/google/mediapipe/blob/ecb5b5f44ab23ea620ef97a479407c699e424aa7/mediapipe/objc/MPPGraphTestBase.mm#L291
/**
 Returns a new pixel buffer if the pixel format of the input isn't the same; otherwise returns the same pixel buffer.
 The returned value is autoreleased. You should retain it yourself if you want to keep it around.
 */
CVPixelBufferRef SKRN_convertPixelBufferToPixelFormat(CVPixelBufferRef input, OSType pixelFormat) {
  size_t width = CVPixelBufferGetWidth(input);
  size_t height = CVPixelBufferGetHeight(input);
  CVPixelBufferRef output;

    OSStatus originalFormat = CVPixelBufferGetPixelFormatType(input);
    if(originalFormat == pixelFormat) {
        CFAutorelease(input);
        return input;
    }
  CVReturn status = CVPixelBufferCreate(
      kCFAllocatorDefault, width, height, pixelFormat,
      SKRN_GetCVPixelBufferAttributesForGlCompatibility(), &output);
    if(status != kCVReturnSuccess) {
        return NULL;
    }

  status = CVPixelBufferLockBaseAddress(input, kCVPixelBufferLock_ReadOnly);
    if(status != kCVReturnSuccess) {
        return NULL;
    }

  status = CVPixelBufferLockBaseAddress(output, 0);
    if(status != kCVReturnSuccess) {
        return NULL;
    }

  status = (CVReturn)SKRN_vImageConvertCVPixelBuffers(input, output);
    if(status != kvImageNoError) {
        return NULL;
    }

  status = CVPixelBufferUnlockBaseAddress(output, 0);
    if(status != kCVReturnSuccess) {
        return NULL;
    }

  status = CVPixelBufferUnlockBaseAddress(input, kCVPixelBufferLock_ReadOnly);
    if(status != kCVReturnSuccess) {
        return NULL;
    }

  return output;
}

NSData *RawRGBA32NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation) {
    UInt32MallocatedPointerStruct res = RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, orientation);
    NSData *ret = [NSData dataWithBytesNoCopy:res.ptr length:res.len];
    return ret;
}

NSData *RawFloat32RGBAScaledNSDataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    Float32MallocatedPointerStruct res = RawFloat32RGBAScaledDataFromDataFromCMSampleBuffer(buffer);
    return [NSData dataWithBytesNoCopy:(void *)res.ptr length:res.len];
}

NSData *RawBGR24NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation) {
    UInt8MallocatedPointerStruct res = RawBGR24DataFromCMSampleBufferAndOrientation(buffer, orientation);
    return [NSData dataWithBytesNoCopy:res.ptr length:res.len];
}
NSData *RawRGB24NSDataFromCMSampleBuffer(CMSampleBufferRef buffer, UIImageOrientation orientation) {
    UInt8MallocatedPointerStruct res = RawRGB24DataFromCMSampleBufferAndOrientation(buffer, orientation);
    return [NSData dataWithBytesNoCopy:res.ptr length:res.len];
}


SKRNNativeVideo::UInt8MallocatedPointerStruct RawBGR24DataFromCMSampleBufferAndOrientation
(
 CMSampleBufferRef buffer, UIImageOrientation orientation
 ) {
    UInt32MallocatedPointerStruct res = RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, orientation);
    // Original is 1 byte per element, 4 elements (RGBA_8888)
    size_t numInElements = res.len;
    size_t numPixels = numInElements/4;
    
    size_t numOutElements = numPixels * 3;
    UInt8 *outBytes = (UInt8 *)malloc(numOutElements);
    for(int i = 0; i < numPixels; i++) {
        UInt32 rgba32 = res.ptr[i];
//        if(i % 360 == 0) {
//            // I feel like I'm not getting something here. seems like I need to bitshift 24 to get alpha
//            // But it's RGBA ?? Why's alpha at the far left?
//            // According to https://stackoverflow.com/a/23304474/4469172
//            // "Little endian by definition stores the bytes of a number in reverse order"
//            // So... RGBA is stored as ABGR?
//            printf("{%d(%d)}", i, (UInt8)((rgba32 >> 24) & 0b11111111)); // alpha should always be 255 right?
//        }
        
        // So... RGBA is stored as ABGR in little endian?
        // B = bitshift out G and R and cast out the rest
        outBytes[i * 3] = (UInt8)((rgba32 >> 16) & 0b11111111);
        // G : ditto
        outBytes[i * 3 + 1] = (UInt8)((rgba32 >> 8) & 0b11111111);
        // R : ditto
        outBytes[i * 3 + 2] = (UInt8)((rgba32) & 0b11111111);
//        if(i % 360 == 0) { // read value every third of 1080 frame
//            printf(",%d:(%d, %d, %d)", i, outBytes[i*3 + 2], outBytes[i*3 + 1], outBytes[i*3]);
//        }
        // OK. Values seem to be correct in ordering now.
    }
    free(res.ptr);
    return (UInt8MallocatedPointerStruct) {
        .ptr = outBytes,
        .len = numOutElements
    };
}
SKRNNativeVideo::UInt8MallocatedPointerStruct RawRGB24DataFromCMSampleBufferAndOrientation
(
 CMSampleBufferRef buffer, UIImageOrientation orientation
 ) {
    // Ditto with BGR24 implementation
    UInt32MallocatedPointerStruct res = RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, orientation);
    // Original is 1 byte per element, 4 elements (RGBA_8888)
    size_t numInElements = res.len;
    size_t numPixels = numInElements/4;
    
    size_t numOutElements = numPixels * 3;
    UInt8 *outBytes = (UInt8 *)malloc(numOutElements);
    for(int i = 0; i < numPixels; i++) {
        UInt32 rgba32 = res.ptr[i];
        // So... RGBA is stored as ABGR in little endian..
        // I suppose this means RGB = out BGR?
        // B = bitshift out G and R and cast out the rest
        outBytes[i * 3] = (UInt8)((rgba32) & 0b11111111);
        // G : ditto
        outBytes[i * 3 + 1] = (UInt8)((rgba32 >> 8) & 0b11111111);
        // R : ditto
        outBytes[i * 3 + 2] = (UInt8)((rgba32 >> 16) & 0b11111111);
//        if(i % 360 == 0) { // read value every third of 1080 frame
//            printf(",%d:(%d, %d, %d)", i, outBytes[i*3 + 2], outBytes[i*3 + 1], outBytes[i*3]);
//        }
        // OK. Values seem to be correct in ordering now.
    }
    free(res.ptr);
    return (UInt8MallocatedPointerStruct) {
        .ptr = outBytes,
        .len = numOutElements
    };

}

UInt8MallocatedPointerStruct RawBGR24DataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    return RawBGR24DataFromCMSampleBufferAndOrientation(buffer, UIImageOrientationUp);
}


Float32MallocatedPointerStruct RawFloat32RGBScaledDataFromDataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    Float32 maxChannelValueScale = 1/255.0;
    UInt32MallocatedPointerStruct res = RawRGBA32DataFromCMSampleBuffer(buffer);
//    size_t stride = sizeof(UInt32); // stride = 4 (it's RGBA32 = 4 bytes)
    size_t numIntermediateElements = res.len; // Num elements = number of bytes exactly component
    size_t numOutElements = res.len * 3/4; // Num elements = 3/4 * number of bytes exactly since we drop the alpha component
    size_t intermediateAllocSize = numIntermediateElements * sizeof(Float32);
    size_t outAllocSize = numOutElements * sizeof(Float32);
    // Copies a vector to another vector (single-precision).
    Float32 *intermediateBuffer = (Float32 *)malloc(intermediateAllocSize);
    Float32 *outBuffer = (Float32 *)malloc(outAllocSize);
    
    // Convert from int8 to float32
    vDSP_vfltu8((unsigned char *)res.ptr, 1, intermediateBuffer, 1, numIntermediateElements);
    
    // Free original pointer
    free(res.ptr);
    
    // Copy channels, one by one
    int numChannelElements = (int)numOutElements/3;
    cblas_scopy(numChannelElements, intermediateBuffer, 4, outBuffer, 3); // Copy R channel
    cblas_scopy(numChannelElements, intermediateBuffer + 1, 4, outBuffer + 1, 3); // Copy G channel
    cblas_scopy(numChannelElements, intermediateBuffer + 2, 4, outBuffer + 2, 3); // Copy B channel
    free(intermediateBuffer);
    // Scale by 1/(maximum channel value)
    vDSP_vsmul(outBuffer, 1, &maxChannelValueScale, outBuffer, 1, numOutElements);
    return (Float32MallocatedPointerStruct) {
        .ptr = outBuffer,
        .len = outAllocSize
    };
}


Float32MallocatedPointerStruct RawFloat32RGBAScaledDataFromDataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    Float32 maxChannelValueScale = 1/255.0;
    UInt32MallocatedPointerStruct res = RawRGBA32DataFromCMSampleBuffer(buffer);
//    size_t stride = sizeof(UInt32); // stride = 4 (it's RGBA32 = 4 bytes)
    size_t numElements = res.len; // Num elements = number of bytes exactly since every component is 1 byte (RGBA = 4 bytes)
    size_t outAllocSize = numElements * sizeof(Float32);
    Float32 *outBuffer = (Float32 *)malloc(outAllocSize);
    // Convert from int8 to float32
    vDSP_vfltu8((unsigned char *)res.ptr, 1, outBuffer, 1, numElements);
    
    free(res.ptr);
    // Scale by 1/(maximum channel value)
    vDSP_vsmul(outBuffer, 1, &maxChannelValueScale, outBuffer, 1, numElements);
    return (Float32MallocatedPointerStruct) {
        .ptr = outBuffer,
        .len = outAllocSize
    };
}

SKRNNativeVideo::UInt32MallocatedPointerStruct RawRGBA32DataFromCMSampleBufferAndOrientation(CMSampleBufferRef buffer, UIImageOrientation orientation) {
    CVImageBufferRef _imageBuffer = CMSampleBufferGetImageBuffer(buffer);
    CIImage *image = [CIImage imageWithCVPixelBuffer:_imageBuffer];
    if(orientation != UIImageOrientationUp) {
        image = [image imageByApplyingOrientation:SKRNNVCGImagePropertyOrientationForUIImageOrientation(orientation)];
    }
    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CIContext *context = [CIContext contextWithOptions:@{
        kCIContextOutputColorSpace: (__bridge id)rgbColorSpace,
        kCIContextUseSoftwareRenderer:@(NO)
    }];
    
    size_t outBytesPerRow = 4 * image.extent.size.width;
    size_t dataSize = outBytesPerRow * image.extent.size.height;
    void *outRenderBytes = malloc(dataSize);
    [context render:image toBitmap:outRenderBytes rowBytes:outBytesPerRow bounds:image.extent format:kCIFormatRGBA8 colorSpace:rgbColorSpace];
    CGColorSpaceRelease(rgbColorSpace);
    
    UInt32MallocatedPointerStruct ret = (UInt32MallocatedPointerStruct){.ptr = (UInt32 *)outRenderBytes, .len = dataSize };
    return ret;

}
UInt32MallocatedPointerStruct RawRGBA32DataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    return RawRGBA32DataFromCMSampleBufferAndOrientation(buffer, UIImageOrientationUp);
}

Float32MallocatedPointerStruct rawDataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    CVImageBufferRef _imageBuffer = CMSampleBufferGetImageBuffer(buffer);
    if(CVPixelBufferIsPlanar(_imageBuffer)) {
        NSLog(@"Unsupported planar buffers");
        // Planar buffers not supported
        return (Float32MallocatedPointerStruct){.ptr = NULL, .len = 0 };
    }
    
    CVPixelBufferRef imageBuffer = SKRN_convertPixelBufferToPixelFormat(_imageBuffer, kCVPixelFormatType_32RGBA);
    if(imageBuffer == NULL) {
        NSLog(@"Could not convert to 32RGBA");
        return (Float32MallocatedPointerStruct){.ptr = NULL, .len = 0 };
    }
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    size_t width = CVPixelBufferGetWidth(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
//    CIImage *image = [CIImage imageWithCVPixelBuffer:imageBuffer];
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(imageBuffer);
    size_t bytesPerPixel;  // is there a generic way to get this from a pixel buffer?
    switch (pixelFormat) {
      case kCVPixelFormatType_32RGBA:
        bytesPerPixel = 4;
        break;
      case kCVPixelFormatType_OneComponent8:
        bytesPerPixel = 1;
        break;
      default:
            printf("Unsupported pixel format %d", pixelFormat);
            // Unsupported pixel format
            return (Float32MallocatedPointerStruct){.ptr = NULL, .len = 0 };
    }
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    size_t dataLength = CVPixelBufferGetDataSize(imageBuffer);
    
    // After doing react-native-tensorflow-lite I found the hard truth that the numBytesPerRow isn't accurate (sometimes has weird padding after row when image is resized, etc)
    size_t expectedByteWidth = width * bytesPerPixel;
    // The number of bytes to skip once end of row is reached
    size_t numToSkip = 0;
    if(bytesPerRow != expectedByteWidth) {
        // Using dataLength directly sometimes doesn't work, in the case of on-device manipulation,
        // sometimes there are extra `buffer pixels` appended to the end of each row. (data is all zeros, and isn't ever present in normal use of UIImage/CGImage)
        numToSkip = bytesPerRow - expectedByteWidth;
//                NSLog(@"needs padding by %lu bytes", numToSkip);
    }
    CFIndex actualDataLength = expectedByteWidth * height;
    size_t allocLen = sizeof(Float32) * 3 * actualDataLength / 4; // Use bytes
    Float32 *outBytes = (Float32 *)malloc(allocLen);
    
    Float32 maximumChannelValue = 255; // 2 ^ 8 - 1
    // TF Note:
    // Iterate over channels individually. Since the order of the channels in memory
    // may vary, we cannot add channels to the float buffer we pass to TF Lite in the
    // order that they are iterated over.
    // My Note : Basically just rearrange in each Float32 byte why say it so verbosely..
    NSUInteger outBytesIndex = 0;
    UInt8 *baseAddress = (UInt8 *)CVPixelBufferGetBaseAddress(imageBuffer);
    
    BOOL bigEndian = NO;
    BOOL alphaFirst = NO;
    UInt8 alphaOffset, redOffset, greenOffset, blueOffset;
    if(bigEndian) {
        alphaOffset = alphaFirst ? 0 : 3;
        redOffset = alphaFirst ? 1 : 0;
        greenOffset = alphaFirst ? 2 : 1;
        blueOffset = alphaFirst ? 3 : 2;
    }
    else {
        alphaOffset = alphaFirst ? 3 : 0;
        redOffset = alphaFirst ? 2 : 3;
        greenOffset = alphaFirst ? 1 : 2;
        blueOffset = alphaFirst ? 1 : 0;
    }
    
    for(NSUInteger byteIndex = 0; byteIndex < dataLength; byteIndex += 4) {
        if(numToSkip != 0 && (byteIndex + numToSkip) % bytesPerRow == 0) { // If at end of row... (check numToSkip first because modulo is expensive)
            byteIndex += numToSkip; // add skip bytes if needed
        }
        Float32 red, green, blue;
        UInt8 channelData[4];
        
        
        red = (Float32)baseAddress[byteIndex + redOffset]/maximumChannelValue;
        green = (Float32)channelData[byteIndex + greenOffset]/maximumChannelValue;
        blue = (Float32)channelData[byteIndex + blueOffset]/maximumChannelValue;
        // Ignore alpha; useless for us lmao

        // grayscale = R * 0.2126 + G * 0.7152 + B * 0.0722
        outBytes[outBytesIndex] = red;
        outBytes[outBytesIndex + 1] = green;
        outBytes[outBytesIndex + 2] = blue;
        outBytesIndex += 3;
    }
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    Float32MallocatedPointerStruct ret = (Float32MallocatedPointerStruct){.ptr = outBytes, .len = outBytesIndex };
    return ret;
}

CMSampleBufferRef CreateCMSampleBufferFromImage(UIImage *_image, UIImageOrientation orientation) {
    CIImage *image = [CIImage imageWithCGImage:_image.CGImage];
    if(orientation != UIImageOrientationUp) {
        image = [image imageByApplyingOrientation:SKRNNVCGImagePropertyOrientationForUIImageOrientation(orientation)];
    }
    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CIContext *context = [CIContext contextWithOptions:@{
        kCIContextOutputColorSpace: (__bridge id)rgbColorSpace,
        kCIContextUseSoftwareRenderer:@(NO)
    }];
    
    size_t outBytesPerRow = 4 * image.extent.size.width;
    size_t dataSize = outBytesPerRow * image.extent.size.height;
    void *outRenderBytes = malloc(dataSize);
    [context render:image toBitmap:outRenderBytes rowBytes:outBytesPerRow bounds:image.extent format:kCIFormatRGBA8 colorSpace:rgbColorSpace];
    CVPixelBufferRef pixelBuffer;
    // We assign the outRenderBytes to the pixelBuffer, and it gets freed via CVPixelBufferReleaseSimpleFreeMallocedBytesCallback later.
    CVPixelBufferCreateWithBytes(NULL, image.extent.size.width, image.extent.size.height, kCVPixelFormatType_32RGBA, outRenderBytes, image.extent.size.width * 4, CVPixelBufferReleaseSimpleFreeMallocedBytesCallback, NULL, NULL, &pixelBuffer);
    CGColorSpaceRelease(rgbColorSpace);
    
    CMVideoFormatDescriptionRef videoInfo = NULL;
    CMVideoFormatDescriptionCreateForImageBuffer(NULL, pixelBuffer, &videoInfo);
    CMTime frameTime = CMTimeMake(1, 60);
    CMSampleTimingInfo timing = {frameTime, frameTime, kCMTimeInvalid};
    
    CMSampleBufferRef ret = NULL;
    OSStatus ok = CMSampleBufferCreateReadyWithImageBuffer(NULL, pixelBuffer, videoInfo, &timing, &ret);
    if(ok != 0) {
        NSLog(@"error creating sample buffer %d", ok);
    }
    
    CFRelease(pixelBuffer);
    CFRelease(videoInfo);
    return ret;
}
