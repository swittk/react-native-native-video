package com.reactnativenativevideo;

import androidx.annotation.NonNull;

import com.facebook.proguard.annotations.DoNotStrip;

import com.facebook.react.bridge.JavaScriptContextHolder;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

/** Legacy bridge entrypoint exercised by the RN 0.73 Monterey example app. */
@DoNotStrip
@ReactModule(name = NativeVideoModule.NAME)
public final class NativeVideoModule extends ReactContextBaseJavaModule {
  public static final String NAME = "NativeVideo";

  static {
    System.loadLibrary("reactnativenativevideo");
  }

  public NativeVideoModule(ReactApplicationContext reactContext) {
    super(reactContext);
  }

  @NonNull
  @Override
  public String getName() {
    return NAME;
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  public double multiply(double a, double b) {
    return a * b;
  }

  @Override
  public void initialize() {
    super.initialize();
    ReactApplicationContext reactContext = getReactApplicationContext();
    JavaScriptContextHolder contextHolder = reactContext.getJavaScriptContextHolder();
    CallInvokerHolderImpl callInvokerHolder =
        (CallInvokerHolderImpl) reactContext.getCatalystInstance().getJSCallInvokerHolder();
    long runtimePointer = contextHolder.get();
    if (runtimePointer != 0 && callInvokerHolder != null) {
      installLegacyBindings(runtimePointer, callInvokerHolder);
    }
  }

  private native void installLegacyBindings(
      long runtimePointer,
      CallInvokerHolderImpl callInvokerHolder);

  private native void installLegacyBindingsNow(long runtimePointer);

  @ReactMethod(isBlockingSynchronousMethod = true)
  public void installBindings() {
    ReactApplicationContext reactContext = getReactApplicationContext();
    reactContext.assertOnJSQueueThread();
    long runtimePointer = reactContext.getJavaScriptContextHolder().get();
    if (runtimePointer != 0) {
      installLegacyBindingsNow(runtimePointer);
    }
  }

  @DoNotStrip
  SKNativeVideoWrapperJavaSide createVideoWrapper(String sourceUri) {
    return new SKNativeVideoWrapperJavaSide(getReactApplicationContext(), sourceUri);
  }
}
