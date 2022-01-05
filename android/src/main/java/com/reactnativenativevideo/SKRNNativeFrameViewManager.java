package com.reactnativenativevideo;

import android.graphics.Bitmap;
import android.widget.ImageView;
import android.util.Log;

import androidx.annotation.NonNull;

import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReadableMap;
import com.facebook.react.uimanager.SimpleViewManager;
import com.facebook.react.uimanager.ThemedReactContext;
import com.facebook.react.uimanager.annotations.ReactProp;
import com.facebook.react.views.image.ReactImageView;

public class SKRNNativeFrameViewManager extends SimpleViewManager<ReactImageView> {
  public static final String REACT_CLASS = "SKRNNativeFrameView";
  ReactApplicationContext mCallerContext;
  public SKRNNativeFrameViewManager(ReactApplicationContext reactContext) {
    mCallerContext = reactContext;
  }

  @ReactProp(name = "frame")
  public void setFrame(ReactImageView view, ReadableMap arg) {
    String ptrStr = arg.getString("nativePtrStr");
    Bitmap bitmap = getBitmapFromStringAddressOfFrameWrapper(ptrStr);
    if(bitmap == null) {
      Log.d("SKRN", "bitmap null");
      return;
    }
    view.setImageBitmap(bitmap);
//    arg.getString("nativePtrStr");
//    view.setImageBitmap();
//    view.setImageBitmap();
//    view.setController();
  }
  public static native Bitmap getBitmapFromStringAddressOfFrameWrapper(String address);

  @NonNull
  @Override
  public String getName() {
    return REACT_CLASS;
  }

  @Override
  public ReactImageView createViewInstance(ThemedReactContext context) {
    return new ReactImageView(context, Fresco.newDraweeControllerBuilder(), null, mCallerContext);
  }
}
