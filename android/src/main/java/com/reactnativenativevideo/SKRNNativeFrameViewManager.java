package com.reactnativenativevideo;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.uimanager.SimpleViewManager;
import com.facebook.react.uimanager.ThemedReactContext;
import com.facebook.react.uimanager.annotations.ReactProp;

final class SKRNNativeFrameViewManager
    extends SimpleViewManager<SKRNNativeFrameViewManager.NativeFrameImageView> {
  static final String REACT_CLASS = "SKRNNativeFrameView";

  static {
    System.loadLibrary("reactnativenativevideo");
  }

  SKRNNativeFrameViewManager(ReactApplicationContext ignoredReactContext) {}

  @ReactProp(name = "frameId")
  public void setFrameId(NativeFrameImageView view, @Nullable String frameId) {
    view.setNativeFrameId(frameId);
  }

  @ReactProp(name = "resizeMode")
  public void setResizeMode(NativeFrameImageView view, @Nullable String resizeMode) {
    if ("cover".equals(resizeMode)) {
      view.setScaleType(ImageView.ScaleType.CENTER_CROP);
    } else if ("stretch".equals(resizeMode)) {
      view.setScaleType(ImageView.ScaleType.FIT_XY);
    } else {
      view.setScaleType(ImageView.ScaleType.FIT_CENTER);
    }
  }

  private static native long acquireNativeFrame(String nativeId);

  private static native Bitmap bitmapForNativeFrame(long nativeFrame);

  private static native void releaseNativeFrame(long nativeFrame);

  @NonNull
  @Override
  public String getName() {
    return REACT_CLASS;
  }

  @NonNull
  @Override
  protected NativeFrameImageView createViewInstance(
      @NonNull ThemedReactContext context) {
    NativeFrameImageView view = new NativeFrameImageView(context);
    view.setScaleType(ImageView.ScaleType.FIT_CENTER);
    return view;
  }

  @Override
  public void onDropViewInstance(@NonNull NativeFrameImageView view) {
    view.clearNativeFrame();
    super.onDropViewInstance(view);
  }

  @SuppressLint({"AppCompatCustomView", "ViewConstructor"})
  static final class NativeFrameImageView extends ImageView {
    @Nullable private String nativeFrameId;
    private long nativeFrame = 0;

    NativeFrameImageView(ThemedReactContext context) {
      super(context);
    }

    void setNativeFrameId(@Nullable String frameId) {
      if (frameId == null || frameId.isEmpty()) {
        nativeFrameId = null;
        clearNativeFrame();
        return;
      }
      if (frameId.equals(nativeFrameId) && nativeFrame != 0) {
        return;
      }
      clearNativeFrame();
      nativeFrameId = frameId;
      bindNativeFrame();
    }

    void clearNativeFrame() {
      if (nativeFrame != 0) {
        releaseNativeFrame(nativeFrame);
        nativeFrame = 0;
      }
      setImageDrawable(null);
    }

    private void bindNativeFrame() {
      if (nativeFrameId == null || nativeFrame != 0) {
        return;
      }
      nativeFrame = acquireNativeFrame(nativeFrameId);
      setImageBitmap(
          nativeFrame == 0 ? null : bitmapForNativeFrame(nativeFrame));
    }

    @Override
    protected void onAttachedToWindow() {
      super.onAttachedToWindow();
      bindNativeFrame();
    }

    @Override
    protected void onDetachedFromWindow() {
      clearNativeFrame();
      super.onDetachedFromWindow();
    }
  }
}
