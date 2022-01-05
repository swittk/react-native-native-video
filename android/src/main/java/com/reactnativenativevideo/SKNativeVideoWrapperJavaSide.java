package com.reactnativenativevideo;

import android.graphics.Bitmap;
import android.media.MediaMetadataRetriever;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;

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
  }

  @RequiresApi(api = Build.VERSION_CODES.P)
  Bitmap getFrameAtIndex(int index) {
    return mediaRetriever.getFrameAtIndex(index);
  }

  Bitmap getFrameAtTime(double time) {
    // time is in microseconds due to some weird reason
    return mediaRetriever.getFrameAtTime((long) (time * 1000000));
  }

  @RequiresApi(api = Build.VERSION_CODES.P)
  List<Bitmap> getFramesAtIndex(int index, int len) {
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
