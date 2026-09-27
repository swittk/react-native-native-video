package com.reactnativenativevideo;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.Image;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.Build;
import android.hardware.HardwareBuffer;

import androidx.annotation.Nullable;

import com.facebook.proguard.annotations.DoNotStrip;

import java.util.Collections;

/**
 * A decoded Android frame whose primary representation may be a hardware buffer.
 *
 * On API 29+ the decoder hands us an ImageReader-backed Image first. Native C++
 * acquires an independent AHardwareBuffer reference immediately and then calls
 * releaseImage(), so ImageReader slots are never held hostage by long-lived JS
 * frame objects. CPU pixels are decoded lazily only if a caller explicitly asks
 * for arrayBuffer/base64/NativeFrameView.
 */
@DoNotStrip
final class SKAndroidNativeFrameJavaSide {
  private final Context appContext;
  private final String sourceUri;
  private final int frameIndex;
  private final long timestampUs;
  private final int displayWidth;
  private final int displayHeight;
  private final boolean hardwareBacked;

  @Nullable private Image image;
  @Nullable private Bitmap bitmap;

  private SKAndroidNativeFrameJavaSide(
      Context context,
      String sourceUri,
      int frameIndex,
      long timestampUs,
      int displayWidth,
      int displayHeight,
      @Nullable Image image,
      @Nullable Bitmap bitmap,
      boolean hardwareBacked) {
    this.appContext = context.getApplicationContext();
    this.sourceUri = sourceUri;
    this.frameIndex = frameIndex;
    this.timestampUs = timestampUs;
    this.displayWidth = displayWidth;
    this.displayHeight = displayHeight;
    this.image = image;
    this.bitmap = bitmap;
    this.hardwareBacked = hardwareBacked;
  }

  static SKAndroidNativeFrameJavaSide fromHardwareImage(
      Context context,
      String sourceUri,
      int frameIndex,
      long timestampUs,
      int displayWidth,
      int displayHeight,
      Image image) {
    return new SKAndroidNativeFrameJavaSide(
        context,
        sourceUri,
        frameIndex,
        timestampUs,
        displayWidth,
        displayHeight,
        image,
        null,
        true);
  }

  static SKAndroidNativeFrameJavaSide fromBitmap(
      Context context,
      String sourceUri,
      int frameIndex,
      long timestampUs,
      int displayWidth,
      int displayHeight,
      Bitmap bitmap) {
    return new SKAndroidNativeFrameJavaSide(
        context,
        sourceUri,
        frameIndex,
        timestampUs,
        displayWidth,
        displayHeight,
        null,
        ensureArgb8888(bitmap),
        false);
  }

  @DoNotStrip
  synchronized @Nullable HardwareBuffer getHardwareBuffer() {
    if (!hardwareBacked || image == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
      return null;
    }
    try {
      return image.getHardwareBuffer();
    } catch (IllegalStateException ignored) {
      return null;
    }
  }

  @DoNotStrip
  synchronized void releaseImage() {
    if (image != null) {
      image.close();
      image = null;
    }
  }

  @DoNotStrip
  synchronized @Nullable Bitmap getBitmap() {
    if (bitmap != null) {
      return bitmap;
    }
    bitmap = decodeBitmap(appContext, sourceUri, frameIndex, timestampUs);
    return bitmap;
  }

  @DoNotStrip
  int getWidth() {
    return displayWidth;
  }

  @DoNotStrip
  int getHeight() {
    return displayHeight;
  }

  @DoNotStrip
  boolean isHardwareBacked() {
    return hardwareBacked;
  }

  @DoNotStrip
  synchronized void close() {
    releaseImage();
    bitmap = null;
  }

  private static @Nullable Bitmap decodeBitmap(
      Context context,
      String sourceUri,
      int frameIndex,
      long timestampUs) {
    MediaMetadataRetriever retriever = new MediaMetadataRetriever();
    try {
      setRetrieverDataSource(retriever, context, sourceUri);
      Bitmap bitmap = null;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && frameIndex >= 0) {
        try {
          bitmap = retriever.getFrameAtIndex(frameIndex);
        } catch (IllegalArgumentException | IllegalStateException ignored) {
          // Some vendor retrievers only support time-based seeking.
        }
      }
      if (bitmap == null) {
        bitmap = retriever.getFrameAtTime(
            timestampUs,
            MediaMetadataRetriever.OPTION_CLOSEST);
      }
      return ensureArgb8888(bitmap);
    } catch (RuntimeException ignored) {
      return null;
    } finally {
      try {
        retriever.release();
      } catch (Exception ignored) {
        // Already unusable; no recovery action.
      }
    }
  }

  private static void setRetrieverDataSource(
      MediaMetadataRetriever retriever,
      Context context,
      String sourceUri) {
    Uri uri = Uri.parse(sourceUri);
    String scheme = uri.getScheme();
    if ("content".equalsIgnoreCase(scheme)
        || "android.resource".equalsIgnoreCase(scheme)) {
      retriever.setDataSource(context, uri);
    } else if ("file".equalsIgnoreCase(scheme)) {
      retriever.setDataSource(uri.getPath());
    } else if (scheme != null && !scheme.isEmpty()) {
      retriever.setDataSource(sourceUri, Collections.emptyMap());
    } else {
      retriever.setDataSource(sourceUri);
    }
  }

  private static @Nullable Bitmap ensureArgb8888(@Nullable Bitmap bitmap) {
    if (bitmap == null) {
      return null;
    }
    if (bitmap.getConfig() == Bitmap.Config.ARGB_8888) {
      return bitmap;
    }
    return bitmap.copy(Bitmap.Config.ARGB_8888, false);
  }
}
