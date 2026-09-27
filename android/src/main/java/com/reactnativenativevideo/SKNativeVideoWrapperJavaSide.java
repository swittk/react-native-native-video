package com.reactnativenativevideo;

import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.hardware.HardwareBuffer;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Base64;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

@DoNotStrip
final class SKNativeVideoWrapperJavaSide {
  private final ReactApplicationContext context;
  private final String sourceUri;
  private final Uri source;
  private final MediaMetadataRetriever mediaRetriever = new MediaMetadataRetriever();
  @Nullable private HardwareFrameDecoder hardwareDecoder;

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
    this.context = context;
    this.sourceUri = sourceUri;
    this.source = Uri.parse(sourceUri);
    setRetrieverDataSource(mediaRetriever, context, sourceUri, source);
    loadVideoTrackMetadata(context, sourceUri, source);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      try {
        hardwareDecoder = new HardwareFrameDecoder(context, sourceUri, source);
      } catch (IOException | RuntimeException ignored) {
        hardwareDecoder = null;
      }
    }
  }

  @DoNotStrip
  static String base64StringForBitmap(Bitmap bitmap, String format) {
    if (bitmap == null) return "";
    Bitmap.CompressFormat compressFormat =
        "jpg".equalsIgnoreCase(format) || "jpeg".equalsIgnoreCase(format)
            ? Bitmap.CompressFormat.JPEG : Bitmap.CompressFormat.PNG;
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    if (!bitmap.compress(compressFormat, 100, output)) return "";
    return Base64.encodeToString(output.toByteArray(), Base64.NO_WRAP);
  }

  @DoNotStrip
  SKAndroidNativeFrameJavaSide getFrameAtIndex(int requestedIndex) {
    ensureOpen();
    int frameCount = getNumFrames();
    if (frameCount <= 0) return null;
    int index = Math.max(0, Math.min(requestedIndex, frameCount - 1));
    long timestampUs = getFrameTimestampUs(index);
    if (timestampUs < 0) return null;
    int displayWidth = getWidth();
    int displayHeight = getHeight();

    HardwareFrameDecoder decoder = hardwareDecoder;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && decoder != null) {
      try {
        Image image = decoder.decodeFrameAtTimestampUs(timestampUs);
        if (image != null) {
          return SKAndroidNativeFrameJavaSide.fromHardwareImage(
              context, sourceUri, index, timestampUs, displayWidth, displayHeight, image);
        }
      } catch (RuntimeException ignored) {
        // Vendor decoder failed: preserve the established CPU fallback.
      }
    }

    Bitmap bitmap = decodeBitmapAtIndex(index, timestampUs);
    return bitmap == null ? null : SKAndroidNativeFrameJavaSide.fromBitmap(
        context, sourceUri, index, timestampUs, displayWidth, displayHeight, bitmap);
  }

  @DoNotStrip
  List<SKAndroidNativeFrameJavaSide> getFramesAtIndex(int requestedIndex, int requestedLength) {
    ensureOpen();
    int frameCount = getNumFrames();
    if (frameCount <= 0 || requestedLength <= 0) return Collections.emptyList();
    int index = Math.max(0, Math.min(requestedIndex, frameCount - 1));
    int length = Math.min(requestedLength, frameCount - index);
    List<SKAndroidNativeFrameJavaSide> result = new ArrayList<>(length);
    for (int offset = 0; offset < length; offset++) {
      // Preserve one list slot per requested frame. Native code derives the
      // frame index/timestamp from the list position and skips null entries.
      result.add(getFrameAtIndex(index + offset));
    }
    return result;
  }

  @DoNotStrip
  int getNumFrames() {
    ensureOpen();
    if (numFrames >= 0) return numFrames;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      numFrames = parseInt(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_VIDEO_FRAME_COUNT), 0);
    } else {
      double fps = getFrameRate();
      double seconds = getDuration();
      numFrames = fps > 0 && seconds > 0 ? Math.max(1, (int) Math.round(fps * seconds)) : 0;
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
    if (frameCount <= 0) return -1;
    if (frameTimesUs.length == frameCount) {
      long requestedUs = (long) (Math.max(0, timeSeconds) * 1_000_000.0);
      int low = 0;
      int high = frameTimesUs.length;
      while (low < high) {
        int middle = low + (high - low) / 2;
        if (frameTimesUs[middle] <= requestedUs) low = middle + 1;
        else high = middle;
      }
      return Math.max(0, Math.min(frameCount - 1, low - 1));
    }
    double fps = getFrameRate();
    if (fps <= 0) return -1;
    int approximate = (int) Math.floor(Math.max(0, timeSeconds) * fps);
    return Math.max(0, Math.min(frameCount - 1, approximate));
  }

  @DoNotStrip
  double getFrameRate() {
    ensureOpen();
    if (!Double.isNaN(frameRate)) return frameRate;
    frameRate = parseDouble(mediaRetriever.extractMetadata(
        MediaMetadataRetriever.METADATA_KEY_CAPTURE_FRAMERATE), 0);
    if (frameRate <= 0 && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      int count = parseInt(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_VIDEO_FRAME_COUNT), 0);
      double seconds = getDuration();
      if (count > 0 && seconds > 0) frameRate = count / seconds;
    }
    return frameRate;
  }

  @DoNotStrip
  double getDuration() {
    ensureOpen();
    if (Double.isNaN(duration)) {
      duration = parseDouble(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_DURATION), 0) / 1000.0;
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
    if (closed) return;
    closed = true;
    HardwareFrameDecoder decoder = hardwareDecoder;
    hardwareDecoder = null;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && decoder != null) decoder.close();
    try {
      mediaRetriever.release();
    } catch (Exception ignored) {
    }
  }

  private Bitmap decodeBitmapAtIndex(int index, long timestampUs) {
    Bitmap bitmap = null;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      try {
        bitmap = mediaRetriever.getFrameAtIndex(index);
      } catch (IllegalArgumentException | IllegalStateException ignored) {
      }
    }
    if (bitmap == null) {
      bitmap = mediaRetriever.getFrameAtTime(timestampUs, MediaMetadataRetriever.OPTION_CLOSEST);
    }
    return ensureArgb8888(bitmap);
  }

  private void loadDimensions() {
    if (width < 0 || height < 0 || rotation == Integer.MIN_VALUE) {
      width = parseInt(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH), 0);
      height = parseInt(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT), 0);
      rotation = Math.floorMod(parseInt(mediaRetriever.extractMetadata(
          MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION), 0), 360);
    }
  }

  private long getFrameTimestampUs(int index) {
    if (index < 0 || index >= getNumFrames()) return -1;
    if (frameTimesUs.length == numFrames) return frameTimesUs[index];
    double fps = getFrameRate();
    return fps > 0 ? (long) ((index / fps) * 1_000_000.0) : -1;
  }

  private void loadVideoTrackMetadata(ReactApplicationContext context, String sourceUri, Uri uri) {
    MediaExtractor extractor = new MediaExtractor();
    try {
      setExtractorDataSource(extractor, context, sourceUri, uri);
      int videoTrackIndex = -1;
      for (int trackIndex = 0; trackIndex < extractor.getTrackCount(); trackIndex++) {
        MediaFormat format = extractor.getTrackFormat(trackIndex);
        String mime = format.getString(MediaFormat.KEY_MIME);
        if (mime != null && mime.startsWith("video/")) {
          videoTrackIndex = trackIndex;
          if (format.containsKey(MediaFormat.KEY_WIDTH)) width = format.getInteger(MediaFormat.KEY_WIDTH);
          if (format.containsKey(MediaFormat.KEY_HEIGHT)) height = format.getInteger(MediaFormat.KEY_HEIGHT);
          if (format.containsKey(MediaFormat.KEY_ROTATION)) {
            rotation = Math.floorMod(format.getInteger(MediaFormat.KEY_ROTATION), 360);
          }
          if (format.containsKey(MediaFormat.KEY_DURATION)) {
            duration = format.getLong(MediaFormat.KEY_DURATION) / 1_000_000.0;
          }
          if (format.containsKey(MediaFormat.KEY_FRAME_RATE)) {
            try {
              frameRate = format.getInteger(MediaFormat.KEY_FRAME_RATE);
            } catch (ClassCastException notInteger) {
              frameRate = format.getFloat(MediaFormat.KEY_FRAME_RATE);
            }
          }
          break;
        }
      }
      if (videoTrackIndex < 0) return;
      extractor.selectTrack(videoTrackIndex);
      ArrayList<Long> times = new ArrayList<>();
      for (long sampleTime = extractor.getSampleTime(); sampleTime >= 0; sampleTime = extractor.getSampleTime()) {
        times.add(sampleTime);
        if (!extractor.advance()) break;
      }
      if (times.isEmpty()) return;
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
      for (int index = 0; index < uniqueTimes.size(); index++) frameTimesUs[index] = uniqueTimes.get(index);
      numFrames = frameTimesUs.length;
      if (frameTimesUs.length > 1) {
        long spanUs = frameTimesUs[frameTimesUs.length - 1] - frameTimesUs[0];
        if (spanUs > 0) frameRate = ((frameTimesUs.length - 1) * 1_000_000.0) / spanUs;
      }
    } catch (IOException | RuntimeException ignored) {
    } finally {
      extractor.release();
    }
  }

  private static void setRetrieverDataSource(
      MediaMetadataRetriever retriever, ReactApplicationContext context, String sourceUri, Uri uri) {
    String scheme = uri.getScheme();
    if ("content".equalsIgnoreCase(scheme) || "android.resource".equalsIgnoreCase(scheme)) {
      retriever.setDataSource(context, uri);
    } else if ("file".equalsIgnoreCase(scheme)) {
      retriever.setDataSource(uri.getPath());
    } else if (scheme != null && !scheme.isEmpty()) {
      retriever.setDataSource(sourceUri, Collections.emptyMap());
    } else {
      retriever.setDataSource(sourceUri);
    }
  }

  private static void setExtractorDataSource(
      MediaExtractor extractor, ReactApplicationContext context, String sourceUri, Uri uri) throws IOException {
    String scheme = uri.getScheme();
    if ("content".equalsIgnoreCase(scheme) || "android.resource".equalsIgnoreCase(scheme)) {
      try (AssetFileDescriptor descriptor = context.getContentResolver().openAssetFileDescriptor(uri, "r")) {
        if (descriptor == null) throw new IOException("Unable to open video URI");
        long length = descriptor.getLength();
        if (length < 0) extractor.setDataSource(descriptor.getFileDescriptor());
        else extractor.setDataSource(descriptor.getFileDescriptor(), descriptor.getStartOffset(), length);
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
    if (closed) throw new IllegalStateException("NativeVideo is closed");
  }

  private static Bitmap ensureArgb8888(Bitmap bitmap) {
    if (bitmap == null) return null;
    if (bitmap.getConfig() == Bitmap.Config.ARGB_8888) return bitmap;
    return bitmap.copy(Bitmap.Config.ARGB_8888, false);
  }

  private static int parseInt(String value, int fallback) {
    if (value == null || value.isEmpty()) return fallback;
    try { return Integer.parseInt(value); } catch (NumberFormatException ignored) { return fallback; }
  }

  private static double parseDouble(String value, double fallback) {
    if (value == null || value.isEmpty()) return fallback;
    try { return Double.parseDouble(value); } catch (NumberFormatException ignored) { return fallback; }
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private static final class HardwareFrameDecoder {
    private static final long CODEC_TIMEOUT_US = 10_000;
    private static final long IMAGE_TIMEOUT_MS = 500;
    private static final long DECODE_TIMEOUT_NS = TimeUnit.SECONDS.toNanos(3);

    private final MediaExtractor extractor = new MediaExtractor();
    private final MediaCodec codec;
    private final ImageReader imageReader;
    private final HandlerThread imageThread;
    private final ArrayBlockingQueue<Image> images = new ArrayBlockingQueue<>(16);
    private boolean closed;

    HardwareFrameDecoder(ReactApplicationContext context, String sourceUri, Uri uri) throws IOException {
      setExtractorDataSource(extractor, context, sourceUri, uri);
      int videoTrackIndex = -1;
      MediaFormat videoFormat = null;
      for (int index = 0; index < extractor.getTrackCount(); index++) {
        MediaFormat format = extractor.getTrackFormat(index);
        String mime = format.getString(MediaFormat.KEY_MIME);
        if (mime != null && mime.startsWith("video/")) {
          videoTrackIndex = index;
          videoFormat = format;
          break;
        }
      }
      if (videoTrackIndex < 0 || videoFormat == null) {
        extractor.release();
        throw new IOException("Asset has no video track");
      }

      extractor.selectTrack(videoTrackIndex);
      int encodedWidth = videoFormat.getInteger(MediaFormat.KEY_WIDTH);
      int encodedHeight = videoFormat.getInteger(MediaFormat.KEY_HEIGHT);
      imageReader = ImageReader.newInstance(
          encodedWidth, encodedHeight, ImageFormat.PRIVATE, 16,
          HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE);

      imageThread = new HandlerThread("RNNativeVideo-HardwareFrames");
      imageThread.start();
      imageReader.setOnImageAvailableListener(reader -> {
        Image image = null;
        try {
          image = reader.acquireNextImage();
          if (image != null && !images.offer(image)) image.close();
        } catch (IllegalStateException ignored) {
          if (image != null) image.close();
        }
      }, new Handler(imageThread.getLooper()));

      String mime = videoFormat.getString(MediaFormat.KEY_MIME);
      codec = MediaCodec.createDecoderByType(mime);
      codec.configure(videoFormat, imageReader.getSurface(), null, 0);
      codec.start();
    }

    synchronized @Nullable Image decodeFrameAtTimestampUs(long targetUs) {
      if (closed) return null;
      drainImages();
      extractor.seekTo(targetUs, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
      codec.flush();
      drainImages();

      MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
      boolean inputEnded = false;
      Image candidate = null;
      long deadline = System.nanoTime() + DECODE_TIMEOUT_NS;
      try {
        while (System.nanoTime() < deadline) {
          if (!inputEnded) {
            int inputIndex = codec.dequeueInputBuffer(CODEC_TIMEOUT_US);
            if (inputIndex >= 0) {
              ByteBuffer input = codec.getInputBuffer(inputIndex);
              int sampleSize = input == null ? -1 : extractor.readSampleData(input, 0);
              if (sampleSize < 0) {
                codec.queueInputBuffer(inputIndex, 0, 0, Math.max(0, targetUs), MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                inputEnded = true;
              } else {
                long sampleTimeUs = extractor.getSampleTime();
                codec.queueInputBuffer(inputIndex, 0, sampleSize, sampleTimeUs, 0);
                extractor.advance();
              }
            }
          }

          int outputIndex = codec.dequeueOutputBuffer(info, CODEC_TIMEOUT_US);
          if (outputIndex == MediaCodec.INFO_TRY_AGAIN_LATER
              || outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED
              || outputIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) continue;
          if (outputIndex < 0) continue;

          boolean eos = (info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
          boolean codecConfig = (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
          long outputPtsUs = info.presentationTimeUs;

          if (!codecConfig && outputPtsUs <= targetUs) {
            codec.releaseOutputBuffer(outputIndex, true);
            Image rendered = pollRenderedImage();
            if (rendered != null) {
              if (candidate != null) candidate.close();
              candidate = rendered;
              if (outputPtsUs == targetUs) return candidate;
            }
          } else {
            codec.releaseOutputBuffer(outputIndex, false);
            if (candidate != null || outputPtsUs > targetUs) return candidate;
          }
          if (eos) return candidate;
        }
        return candidate;
      } catch (RuntimeException exception) {
        if (candidate != null) candidate.close();
        throw exception;
      }
    }

    private @Nullable Image pollRenderedImage() {
      try {
        return images.poll(IMAGE_TIMEOUT_MS, TimeUnit.MILLISECONDS);
      } catch (InterruptedException exception) {
        Thread.currentThread().interrupt();
        return null;
      }
    }

    private void drainImages() {
      Image image;
      while ((image = images.poll()) != null) image.close();
    }

    synchronized void close() {
      if (closed) return;
      closed = true;
      drainImages();
      try { codec.stop(); } catch (RuntimeException ignored) {}
      codec.release();
      imageReader.close();
      extractor.release();
      imageThread.quitSafely();
    }
  }
}
