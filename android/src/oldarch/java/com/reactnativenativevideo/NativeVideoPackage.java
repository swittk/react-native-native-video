package com.reactnativenativevideo;

import androidx.annotation.NonNull;

import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.uimanager.ViewManager;

import java.util.Collections;
import java.util.List;

/** Legacy package registration; native implementation is shared with New Architecture. */
public final class NativeVideoPackage implements ReactPackage {
  @NonNull
  @Override
  public List<NativeModule> createNativeModules(
      @NonNull ReactApplicationContext reactContext) {
    return Collections.singletonList(new NativeVideoModule(reactContext));
  }

  @NonNull
  @Override
  public List<ViewManager> createViewManagers(
      @NonNull ReactApplicationContext reactContext) {
    return Collections.singletonList(
        new SKRNNativeFrameViewManager(reactContext));
  }
}
