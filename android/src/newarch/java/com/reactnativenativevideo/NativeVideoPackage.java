package com.reactnativenativevideo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.facebook.react.BaseReactPackage;
import com.facebook.react.bridge.ModuleSpec;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Lazy TurboModule registration plus the compatibility native frame view. */
public final class NativeVideoPackage extends BaseReactPackage {
  @Nullable
  @Override
  public NativeModule getModule(
      @NonNull String name,
      @NonNull ReactApplicationContext reactContext) {
    return NativeVideoModule.NAME.equals(name)
        ? new NativeVideoModule(reactContext)
        : null;
  }

  @NonNull
  @Override
  public ReactModuleInfoProvider getReactModuleInfoProvider() {
    return () -> {
      Map<String, ReactModuleInfo> moduleInfos = new HashMap<>();
      moduleInfos.put(
          NativeVideoModule.NAME,
          new ReactModuleInfo(
              NativeVideoModule.NAME,
              NativeVideoModule.class.getName(),
              false,
              false,
              false,
              true));
      return moduleInfos;
    };
  }

  @NonNull
  @Override
  protected List<ModuleSpec> getViewManagers(
      @NonNull ReactApplicationContext reactContext) {
    return Collections.singletonList(
        ModuleSpec.viewManagerSpec(
            () -> new SKRNNativeFrameViewManager(reactContext)));
  }
}
