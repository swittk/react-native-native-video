package com.reactnativenativevideo;

import androidx.annotation.NonNull;

import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.interfaces.BindingsInstallerHolder;
import com.facebook.react.turbomodule.core.interfaces.TurboModuleWithJSIBindings;

/** RN 0.83+ entrypoint. React Native owns the runtime and invokes the installer safely. */
@DoNotStrip
@ReactModule(name = NativeVideoModule.NAME)
public final class NativeVideoModule extends NativeVideoSpec
    implements TurboModuleWithJSIBindings {
  public static final String NAME = "NativeVideo";

  static {
    System.loadLibrary("reactnativenativevideo");
  }

  public NativeVideoModule(ReactApplicationContext reactContext) {
    super(reactContext);
  }

  @Override
  public double multiply(double a, double b) {
    return a * b;
  }

  @Override
  @NonNull
  public String getName() {
    return NAME;
  }

  @DoNotStrip
  @Override
  public native BindingsInstallerHolder getBindingsInstaller();

  @DoNotStrip
  SKNativeVideoWrapperJavaSide createVideoWrapper(String sourceUri) {
    return new SKNativeVideoWrapperJavaSide(getReactApplicationContext(), sourceUri);
  }
}
