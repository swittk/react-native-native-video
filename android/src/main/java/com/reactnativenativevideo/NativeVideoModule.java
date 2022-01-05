package com.reactnativenativevideo;
import android.graphics.Bitmap;
import android.util.Log;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.JavaScriptContextHolder;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;
import com.reactnativenativevideo.SKNativeVideoWrapperJavaSide;


@ReactModule(name = NativeVideoModule.NAME)
public class NativeVideoModule extends ReactContextBaseJavaModule {
  public static final String NAME = "NativeVideo";
  private final ReactApplicationContext reactContext;

  public NativeVideoModule(ReactApplicationContext reactContext) {
    super(reactContext);
    this.reactContext = reactContext;
  }

  @Override
  @NonNull
  public String getName() {
    return NAME;
  }

  static {
    try {
      // Used to load the 'native-lib' library on application startup.
      System.loadLibrary("reactnativenativevideo");
    } catch (Exception ignored) {
    }
  }

  // Example method
  // See https://reactnative.dev/docs/native-modules-android
  @ReactMethod
  public void multiply(int a, int b, Promise promise) {
    promise.resolve(nativeMultiply(a, b));
  }

  public static native int nativeMultiply(int a, int b);

  private static native void initialize(long jsiRuntimePointer);

  private static native void cleanup(long jsiRuntimePointer);


  // This method is called automatically (defined in BaseJavaModule.java)
  // "called on the appropriate method when a life cycle event occurs.
  @Override
  public void initialize() {
    loadClassIfNeeded();
    ReactApplicationContext context = this.reactContext;
    JavaScriptContextHolder jsContext = context.getJavaScriptContextHolder();
    NativeVideoModule.initialize(jsContext.get());
  }

  // This method is called automatically (defined in BaseJavaModule.java)
  // "called on the appropriate method when a life cycle event occurs.
  // This method is equivalent to Objective-C's 'invalidate'
  @Override
  public void onCatalystInstanceDestroy() {
    NativeVideoModule.cleanup(this.reactContext.getJavaScriptContextHolder().get());
    // FlexibleHttpModule.cleanup(this.getReactApplicationContext());
  }

  void loadClassIfNeeded() {
    try {
      Log.d("SKRNNativeVideo", "To get class obj thing");
      Class thing = SKNativeVideoWrapperJavaSide.class;
      Log.d("SKRNNativeVideo", "Found class "+thing);
      thing.getClassLoader().loadClass("com.reactnativenativevideo.SKNativeVideoWrapperJavaSide");
      Log.d("SKRNNativeVideo", "Supposedly loaded class");
      this.reactContext.getClassLoader().loadClass("com.reactnativenativevideo.SKNativeVideoWrapperJavaSide");
      Log.d("SKRNNativeVideo", "ReactContext loaded class");
      Class<?> driverClass = Thread.currentThread().getContextClassLoader().loadClass("com.reactnativenativevideo.SKNativeVideoWrapperJavaSide");
//      Class<?> skNativeVideoWrapperJavaSide = ClassLoader.getSystemClassLoader().loadClass("com.reactnativenativevideo.SKNativeVideoWrapperJavaSide");
    } catch (ClassNotFoundException e) {
      e.printStackTrace();
      Log.d("SKRNNativeVideo", "Failed to load SKNativeVideoWrapperJavaSide");
    }
  }
}
