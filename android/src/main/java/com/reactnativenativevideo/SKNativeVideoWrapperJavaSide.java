package com.reactnativenativevideo;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.media.MediaMetadataRetriever;
import android.os.Build;
import android.util.Base64;
import android.util.Log;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;

import java.io.ByteArrayOutputStream;
import java.util.Dictionary;
import java.util.List;
import java.util.Map;

public class SKNativeVideoWrapperJavaSide {
  public
  String _path;
  MediaMetadataRetriever mediaRetriever;

  double FPS = 0;
  double duration = 0;
  int width = 0;
  int height = 0;
  int numFrames = 0;

  public SKNativeVideoWrapperJavaSide(String path) {
    _path = path;

    // Android noob following this answer https://stackoverflow.com/a/45833710/4469172
    mediaRetriever = new MediaMetadataRetriever();
    mediaRetriever.setDataSource(path);
    // Call getNumFrames once because it is needed for frame retrieval.
    getNumFrames();
  }

  // Following here https://stackoverflow.com/a/9224180/4469172
  static String Base64StringForBitmap(Bitmap bitmap, String format) {
    if(format == null || format.length() == 0) {
      format = "png";
    }
    Bitmap.CompressFormat compressFormat = Bitmap.CompressFormat.PNG;
    if(format == "png") { compressFormat = Bitmap.CompressFormat.PNG; }
    else if(format == "jpg" || format == "jpeg") { compressFormat = Bitmap.CompressFormat.JPEG; }

    ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
    bitmap.compress(compressFormat, 100, byteArrayOutputStream);
    byte[] byteArray = byteArrayOutputStream .toByteArray();
    String encoded = Base64.encodeToString(byteArray, Base64.DEFAULT);
    return encoded;
  }

  static byte[] rgbaValuesFromBitmap(Bitmap bitmap)
  {
    ColorMatrix colorMatrix = new ColorMatrix();
    ColorFilter colorFilter = new ColorMatrixColorFilter(
      colorMatrix);
    Bitmap argbBitmap = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(),
      Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(argbBitmap);

    Paint paint = new Paint();

    paint.setColorFilter(colorFilter);
    canvas.drawBitmap(bitmap, 0, 0, paint);

    int width = bitmap.getWidth();
    int height = bitmap.getHeight();
    int componentsPerPixel = 4; // RGBA
    int totalPixels = width * height;
    int totalBytes = totalPixels * componentsPerPixel;

    byte[] rgbValues = new byte[totalBytes];
    @ColorInt int[] argbPixels = new int[totalPixels];
    argbBitmap.getPixels(argbPixels, 0, width, 0, 0, width, height);
    for (int i = 0; i < totalPixels; i++) {
      @ColorInt int argbPixel = argbPixels[i];
      int red = Color.red(argbPixel);
      int green = Color.green(argbPixel);
      int blue = Color.blue(argbPixel);
      int alpha = Color.alpha(argbPixel);
      rgbValues[i * componentsPerPixel + 0] = (byte) red;
      rgbValues[i * componentsPerPixel + 1] = (byte) green;
      rgbValues[i * componentsPerPixel + 2] = (byte) blue;
      rgbValues[i * componentsPerPixel + 3] = (byte) alpha;
    }
    return rgbValues;
  }


  @RequiresApi(api = Build.VERSION_CODES.P)
  Bitmap getFrameAtIndex(int index) {
    if(index < 0) index = 0;
    else if(index > numFrames - 1) index = numFrames - 1;
    return mediaRetriever.getFrameAtIndex(index);
  }

  Bitmap getFrameAtTime(double time) {
    // time is in microseconds due to some weird reason
    return mediaRetriever.getFrameAtTime((long) (time * 1000000));
  }

  @RequiresApi(api = Build.VERSION_CODES.P)
  List<Bitmap> getFramesAtIndex(int index, int len) {
    if(index < 0) index = 0;
    else if(index > numFrames - 1) index = numFrames - 1;
    if(index + len > numFrames - 1) {
      len = (numFrames - 1) - index;
    }
    return mediaRetriever.getFramesAtIndex(index, len);
  }

  int getNumFrames() {
    if(numFrames != 0) {
      return numFrames;
    }
    String fcStr = mediaRetriever.extractMetadata(mediaRetriever.METADATA_KEY_VIDEO_FRAME_COUNT);
    if(fcStr == null || fcStr.isEmpty()) {
      return 0;
    }
    numFrames = Integer.parseInt(fcStr);
    Log.d("NumFrames", "NumFrames" + numFrames);
    return numFrames;
  }

  double getFrameRate() {
    if(FPS != 0) { return FPS; }

    String capFrameRateString = mediaRetriever.extractMetadata(mediaRetriever.METADATA_KEY_CAPTURE_FRAMERATE);
    if(capFrameRateString == null) {
      FPS = (double)(getNumFrames())/getDuration();
    }
    else {
      FPS = Double.parseDouble(capFrameRateString);
    }
    Log.d("SKRN", "FPS" + FPS);
    return FPS;
  }
  double getDuration() {
    if(duration != 0) { return duration; }
    String dur = mediaRetriever.extractMetadata(mediaRetriever.METADATA_KEY_DURATION);
    if(dur == null || dur.isEmpty()) {
      Log.d("SKRN", "Empty duration");
      return 0;
    }
    int durationMilliseconds = Integer.parseInt(dur);
    duration = (double)(durationMilliseconds)/1000.0f;
    Log.d("SKRN", "getDuration" + duration);
    return duration;
  }
  int getWidth() {
    if(width != 0) {return width;}
    String wStr = mediaRetriever.extractMetadata(mediaRetriever.METADATA_KEY_VIDEO_WIDTH);
    if(wStr == null || wStr.isEmpty()) {
      return 0;
    }
    width = Integer.parseInt(wStr);
    Log.d("SKRN", "width" + width);
    return width;
  }
  int getHeight() {
    if(height != 0) {return height;}
    String hStr = mediaRetriever.extractMetadata(mediaRetriever.METADATA_KEY_VIDEO_HEIGHT);
    if(hStr == null || hStr.isEmpty()) {return 0;}
    height = Integer.parseInt(hStr);
    Log.d("SKRN", "height" + height);
    return height;
  }
}
