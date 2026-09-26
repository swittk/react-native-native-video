package com.reactnativenativevideo;

import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.Build;
import android.util.Base64;

import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Platform decoder adapter. Pixel access itself stays native through AndroidBitmap. */
@DoNotStrip
final class SKNativeVideoWrapperJavaSide {
  private final MediaMetadataRetriever mediaRetriever = new MediaMetadataRetriever();

  private boolean closed = false;
  private double frameRate = Double.NaN;
  private double duration = Double.NaN;
  private int width = -1;
  private int height = -1;
  private int numFrames = -1;
  private int rotation = Integer.MIN_VALUE;
  private long[] frameTimesUs = new long[0];

  @DoNotStrip
  SKNativeVideoWrapperJavaSide(ReactApplicationContext context, String sourceUri) {
    Uri uri = Uri.parse(sourceUri);
    String scheme = uri.getScheme();
    if ("content".equalsIgnoreCase(scheme)
        || "android.resource".equalsIgnoreCase(scheme)) {
      mediaRetriever.setDataSource(context, uri);
    } else if ("file".equalsIgnoreCase(scheme)) {
      mediaRetriever.setDataSource(uri.getPath());
    } else if (scheme != null && !scheme.isEmpty()) {
      mediaRetriever.setDataSource(sourceUri, Collections.emptyMap());
    } else {
      mediaRetriever.setDataSource(sourceUri);
    }
    loadVideoTrackMetadata(context, sourceUri, uri);
  }

  @DoNotStrip
  static String base64StringForBitmap(Bitmap bitmap, String format) {
    Bitmap.CompressFormat compressFormat =
        "jpg".equalsIgnoreCase(format) || "jpeg".equalsIgnoreCase(format)
            ? Bitmap.CompressFormat.JPEG
            : Bitmap.CompressFormat.PNG;
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    if (!bitmap.compress(compressFormat, 100, output)) {
      return "";
    }
    return Base64.encodeToString(output.toByteArray(), Base64.NO_WRAP);
  }

  @DoNotStrip
  Bitmap getFrameAtIndex(int requestedIndex) {
    ensureOpen();
    int frameCount = getNumFrames();
    if (frameCount <= 0) {
      return null;
    }
    int index = Math.max(0, Math.min(requestedIndex, frameCount - 1));
    Bitmap bitmap = null;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      try {
        bitmap = mediaRetriever.getFrameAtIndex(index);
      } catch (IllegalArgumentException | IllegalStateException ignored) {
        // Some vendor retrievers only support time-based seeking.
      }
    }
    if (bitmap == null) {
      long timestampUs = getFrameTimestampUs(index);
      if (timestampUs < 0) {
        return null;
      }
      bitmap = mediaRetriever.getFrameAtTime(
          timestampUs,
          MediaMetadataRetriever.OPTION_CLOSEST);
    }
    return ensureArgb8888(bitmap);
  }

  @DoNotStrip
  List<Bitmap> getFramesAtIndex(int requestedIndex, int requestedLength) {
    ensureOpen();
    int frameCount = getNumFrames();
    if (frameCount <= 0 || requestedLength <= 0) {
      return Collections.emptyList();
    }
    int index = Math.max(0, Math.min(requestedIndex, frameCount - 1));
    int length = Math.min(requestedLength, frameCount - index);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      try {
        List<Bitmap> decoded = mediaRetriever.getFramesAtIndex(index, length);
        List<Bitmap> converted = new ArrayList<>(decoded.size());
        for (Bitmap bitmap : decoded) {
          Bitmap rgba = ensureArgb8888(bitmap);
          if (rgba != null) {
            converted.add(rgba);
          }
        }
        return converted;
      } catch (IllegalArgumentException | IllegalStateException ignored) {
        // Fall through to the portable time-based path.
      }
    }

    List<Bitmap> result = new ArrayList<>(length);
    for (int offset = 0; offset < length; offset++) {
      Bitmap bitmap = getFrameAtIndex(index + offset);
      if (bitmap != null) {
        result.add(bitmap);
      }
    }
    return result;
  }

  @DoNotStrip
  int getNumFrames() {
    ensureOpen();
    if (numFrames >= 0) {
      return numFrames;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      numFrames = parseInt(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_VIDEO_FRAME_COUNT),
          0);
    } else {
      double fps = getFrameRate();
      double seconds = getDuration();
      numFrames = fps > 0 && seconds > 0
          ? Math.max(1, (int) Math.round(fps * seconds))
          : 0;
    }
    return numFrames;
  }

  @DoNotStrip
  double getFrameTimestampAtIndex(int index) {
    ensureOpen();
    long timestampUs = getFrameTimestampUs(index);
    return timestampUs < 0 ? 0 : timestampUs / 1_000_000.0;
  }

  @DoNotStrip
  int getFrameIndexAtTime(double timeSeconds) {
    ensureOpen();
    int frameCount = getNumFrames();
    if (frameCount <= 0) {
      return -1;
    }
    if (frameTimesUs.length == frameCount) {
      long requestedUs = (long) (Math.max(0, timeSeconds) * 1_000_000.0);
      int low = 0;
      int high = frameTimesUs.length;
      while (low < high) {
        int middle = low + (high - low) / 2;
        if (frameTimesUs[middle] <= requestedUs) {
          low = middle + 1;
        } else {
          high = middle;
        }
      }
      return Math.max(0, Math.min(frameCount - 1, low - 1));
    }
    double fps = getFrameRate();
    if (fps <= 0) {
      return -1;
    }
    int approximate = (int) Math.floor(Math.max(0, timeSeconds) * fps);
    return Math.max(0, Math.min(frameCount - 1, approximate));
  }

  @DoNotStrip
  double getFrameRate() {
    ensureOpen();
    if (!Double.isNaN(frameRate)) {
      return frameRate;
    }
    frameRate = parseDouble(
        mediaRetriever.extractMetadata(
            MediaMetadataRetriever.METADATA_KEY_CAPTURE_FRAMERATE),
        0);
    if (frameRate <= 0 && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      int count = parseInt(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_VIDEO_FRAME_COUNT),
          0);
      double seconds = getDuration();
      if (count > 0 && seconds > 0) {
        frameRate = count / seconds;
      }
    }
    return frameRate;
  }

  @DoNotStrip
  double getDuration() {
    ensureOpen();
    if (Double.isNaN(duration)) {
      duration = parseDouble(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_DURATION),
          0) / 1000.0;
    }
    return duration;
  }

  @DoNotStrip
  int getWidth() {
    ensureOpen();
    loadDimensions();
    return rotation == 90 || rotation == 270 ? height : width;
  }

  @DoNotStrip
  int getHeight() {
    ensureOpen();
    loadDimensions();
    return rotation == 90 || rotation == 270 ? width : height;
  }

  @DoNotStrip
  void close() {
    if (!closed) {
      closed = true;
      try {
        mediaRetriever.release();
      } catch (Exception ignored) {
        // The decoder is already unusable and there is no recovery action.
      }
    }
  }

  private void loadDimensions() {
    if (width < 0 || height < 0 || rotation == Integer.MIN_VALUE) {
      width = parseInt(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH),
          0);
      height = parseInt(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT),
          0);
      rotation = parseInt(
          mediaRetriever.extractMetadata(
              MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION),
          0);
      rotation = Math.floorMod(rotation, 360);
    }
  }

  private long getFrameTimestampUs(int index) {
    if (index < 0 || index >= getNumFrames()) {
      return -1;
    }
    if (frameTimesUs.length == numFrames) {
      return frameTimesUs[index];
    }
    double fps = getFrameRate();
    return fps > 0 ? (long) ((index / fps) * 1_000_000.0) : -1;
  }

  /**
   * Builds an exact presentation-time index without decoding frame pixels.
   * MediaMetadataRetriever remains the platform decoder; MediaExtractor only
   * supplies backend-independent timing and track metadata.
   */
  private void loadVideoTrackMetadata(
      ReactApplicationContext context,
      String sourceUri,
      Uri uri) {
    MediaExtractor extractor = new MediaExtractor();
    try {
      setExtractorDataSource(extractor, context, sourceUri, uri);
      int videoTrackIndex = -1;
      for (int trackIndex = 0; trackIndex < extractor.getTrackCount(); trackIndex++) {
        MediaFormat format = extractor.getTrackFormat(trackIndex);
        String mime = format.getString(MediaFormat.KEY_MIME);
        if (mime != null && mime.startsWith("video/")) {
          videoTrackIndex = trackIndex;
          if (format.containsKey(MediaFormat.KEY_WIDTH)) {
            width = format.getInteger(MediaFormat.KEY_WIDTH);
          }
          if (format.containsKey(MediaFormat.KEY_HEIGHT)) {
            height = format.getInteger(MediaFormat.KEY_HEIGHT);
          }
          if (format.containsKey(MediaFormat.KEY_DURATION)) {
            duration = format.getLong(MediaFormat.KEY_DURATION) / 1_000_000.0;
          }
          if (format.containsKey(MediaFormat.KEY_FRAME_RATE)) {
            frameRate = format.getInteger(MediaFormat.KEY_FRAME_RATE);
          }
          break;
        }
      }
      if (videoTrackIndex < 0) {
        return;
      }

      extractor.selectTrack(videoTrackIndex);
      ArrayList<Long> times = new ArrayList<>();
      for (long sampleTime = extractor.getSampleTime();
          sampleTime >= 0;
          sampleTime = extractor.getSampleTime()) {
        times.add(sampleTime);
        if (!extractor.advance()) {
          break;
        }
      }
      if (times.isEmpty()) {
        return;
      }
      Collections.sort(times);
      ArrayList<Long> uniqueTimes = new ArrayList<>(times.size());
      long previous = Long.MIN_VALUE;
      for (long time : times) {
        if (time != previous) {
          uniqueTimes.add(time);
          previous = time;
        }
      }
      frameTimesUs = new long[uniqueTimes.size()];
      for (int index = 0; index < uniqueTimes.size(); index++) {
        frameTimesUs[index] = uniqueTimes.get(index);
      }
      numFrames = frameTimesUs.length;
      if (frameTimesUs.length > 1) {
        long spanUs = frameTimesUs[frameTimesUs.length - 1] - frameTimesUs[0];
        if (spanUs > 0) {
          frameRate = ((frameTimesUs.length - 1) * 1_000_000.0) / spanUs;
        }
      }
    } catch (IOException | RuntimeException ignored) {
      // The retriever remains usable even when a vendor extractor cannot index.
    } finally {
      extractor.release();
    }
  }

  private static void setExtractorDataSource(
      MediaExtractor extractor,
      ReactApplicationContext context,
      String sourceUri,
      Uri uri) throws IOException {
    String scheme = uri.getScheme();
    if ("content".equalsIgnoreCase(scheme)
        || "android.resource".equalsIgnoreCase(scheme)) {
      try (AssetFileDescriptor descriptor =
          context.getContentResolver().openAssetFileDescriptor(uri, "r")) {
        if (descriptor == null) {
          throw new IOException("Unable to open video URI");
        }
        long length = descriptor.getLength();
        if (length < 0) {
          extractor.setDataSource(descriptor.getFileDescriptor());
        } else {
          extractor.setDataSource(
              descriptor.getFileDescriptor(), descriptor.getStartOffset(), length);
        }
      }
    } else if ("file".equalsIgnoreCase(scheme)) {
      extractor.setDataSource(uri.getPath());
    } else if (scheme != null && !scheme.isEmpty()) {
      extractor.setDataSource(sourceUri, Collections.emptyMap());
    } else {
      extractor.setDataSource(sourceUri);
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("NativeVideo is closed");
    }
  }

  private static Bitmap ensureArgb8888(Bitmap bitmap) {
    if (bitmap == null) {
      return null;
    }
    if (bitmap.getConfig() == Bitmap.Config.ARGB_8888) {
      return bitmap;
    }
    return bitmap.copy(Bitmap.Config.ARGB_8888, false);
  }

  private static int parseInt(String value, int fallback) {
    if (value == null || value.isEmpty()) {
      return fallback;
    }
    try {
      return Integer.parseInt(value);
    } catch (NumberFormatException ignored) {
      return fallback;
    }
  }

  private static double parseDouble(String value, double fallback) {
    if (value == null || value.isEmpty()) {
      return fallback;
    }
    try {
      return Double.parseDouble(value);
    } catch (NumberFormatException ignored) {
      return fallback;
    }
  }
}
