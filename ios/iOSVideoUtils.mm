//
//  iOSVideoUtils.cpp
//  react-native-native-video
//
//  Created by Switt Kongdachalert on 4/1/2565 BE.
//

#include "iOSVideoUtils.h"
using namespace SKRNNativeVideo;

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

Float32MallocatedPointerStruct rawDataFromCMSampleBuffer(CMSampleBufferRef buffer) {
    CVImageBufferRef _imageBuffer = CMSampleBufferGetImageBuffer(buffer);
    if(CVPixelBufferIsPlanar(_imageBuffer)) {
        // Planar buffers not supported
        return (Float32MallocatedPointerStruct){.ptr = NULL, .len = 0 };
    }
    CVPixelBufferRef imageBuffer = SKRN_convertPixelBufferToPixelFormat(_imageBuffer, kCVPixelFormatType_32RGBA);
    if(imageBuffer == NULL) {
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
