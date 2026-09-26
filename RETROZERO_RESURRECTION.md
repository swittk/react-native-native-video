# RetroZero NativeVideo resurrection

## Owner baseline and compatibility contract

This work targets the final owner baseline in RetroZero's `BULLETIN.md`:

- Expo SDK 55
- React Native 0.83.10 / React 19.2
- RetroZero application deployment target iOS 15.1
- no owner-authored Swift

The package remains one implementation with two React Native entry paths. RN
0.83 uses supported New Architecture binding installers. The retained RN
0.64.3 example uses narrowly isolated legacy runtime access. Both paths install
the same C++ JSI HostObject factory and share the same platform decoders, frame
objects, native preview views, and resource-lifetime rules.

The podspec keeps the package's existing iOS 10.0 floor. That library floor is
separate from RetroZero's iOS 15.1 application floor and still needs an Xcode
build on the owner's Monterey-compatible example before it is considered fully
proven.

## Preserved media architecture

NativeVideo remains a platform-codec/direct-frame library. It does not use
FFmpeg as its primary backend and does not write one file per decoded frame.

- iOS uses `AVAssetReader` and retains decoded `CMSampleBufferRef` frames.
- Android uses `MediaMetadataRetriever` for platform frame decoding and
  `MediaExtractor` for the video track's presentation timestamp map.
- C++ JSI HostObjects expose video and frame identity synchronously.
- `arrayBuffer()` returns display-oriented, tightly packed RGBA8 bytes by
  copying native decoded pixels directly into a JavaScript `ArrayBuffer`. There
  is no intermediate image file; this is not advertised as an external-buffer
  or zero-copy JS allocation.
- Video metadata includes source URI, duration, display size, frame count, and
  average/nominal frame rate.
- Frames preserve zero-based index, presentation timestamp, dimensions,
  `bytesPerRow`, pixel format, platform, raw bytes, encoding helpers, and
  explicit idempotent `close()` lifetime control.
- `getFrameTimestampAtIndex()` and `getFrameIndexAtTime()` expose the same map
  used by `getFrameAtTime()`. The time lookup selects the frame at or
  immediately before the requested presentation time.
- Android uses exact extracted presentation times when the platform extractor
  can index the source. It falls back to duration/frame-rate-derived timestamps
  on sources or vendor implementations that cannot be indexed.
- Native preview views resolve opaque frame IDs through a weak C++ registry and
  retain a `shared_ptr` while mounted. `nativePtrStr` remains only as a
  compatibility alias and no longer serializes a dereferenceable pointer.

The shared backend-independent C++ video/frame interfaces are the extension
seam for later trim, crop, scale, rotate, concat, speed, audio replace/mix,
encode, mux, and transcode work. None of those operations is falsely claimed in
this resurrection. An optional FFmpeg backend can be added later for operations
the platform stacks cannot provide without changing frame identity or the
public architecture.

## RN 0.83 New Architecture install path

### iOS

`ios/NativeVideo.h` conditionally conforms to the codegen-generated
`NativeVideoSpec` when `RCT_NEW_ARCH_ENABLED` is set. `ios/NativeVideo.mm`
returns `NativeVideoSpecJSI` and implements
`RCTTurboModuleWithJSIBindings::installJSIBindingsWithRuntime:callInvoker:`.
React Native owns and supplies the JSI runtime; the package does not reach into
private bridge runtime state on this path.

The Objective-C++ AVFoundation backend asynchronously loads the video track,
scans exact sample presentation times without decoding pixels, and then uses a
random-access `AVAssetReaderTrackOutput` for native frame decode. All owner
implementation remains Objective-C++/C++.

### Android

RN 0.82+ selects `android/src/newarch/java`. The module extends the generated
`NativeVideoSpec`, implements `TurboModuleWithJSIBindings`, and returns React
Native's `BindingsInstallerHolder`. The installer captures module state, then
installs the common HostObject factory into the runtime supplied by RN.

Gradle resolves React Native from the consuming application's `node_modules`,
applies the React plugin/codegen only on the modern path, and uses RN 0.83's
Prefab packages. CMake links `ReactAndroid::jsi`,
`ReactAndroid::reactnative`, and `fbjni::fbjni` instead of reaching into RN
source-tree internals. Modern defaults are compile/target SDK 36, min SDK 24,
NDK 27.1, Java 17, and C++20; host projects can override the normal Gradle
properties.

## Retained RN 0.64 example path

The actual example remains React 17 / RN 0.64.3 / Expo 44 so it can continue to
serve the owner's Monterey-era workflow.

- iOS conditionally registers the same class as an `RCTBridgeModule`. Only this
  fallback observes `RCTJavaScriptDidLoadNotification` and obtains the ready
  `RCTCxxBridge` runtime. It tracks runtime replacement and removes observers
  on invalidation/deallocation.
- Android selects `android/src/oldarch/java`, a normal `ReactPackage`, and a
  bridge module. Only this fallback reads `JavaScriptContextHolder` and calls
  the legacy JNI installer.
- The old Android path uses RN 0.64 headers/C++17 and includes `jsi.cpp` where
  that release requires it. The media core and JS API are not forked.

`openVideo()` is therefore not New-Architecture-only. Importing the package
resolves the small `NativeVideo` module, which causes the appropriate installer
to run for the host architecture.

## RetroZero consumption

Expose `react-native-native-video` as a normal lazy ComponentOnce external. The
external registry should load this package only when a package actually asks
for it; do not eagerly `require()` NativeVideo with the entire native-module
catalog. Once lazily loaded, the package import intentionally resolves the
TurboModule so React Native invokes its supported bindings installer before
`openVideo()` is called.

After adding the local/package dependency, regenerate the Expo native project
and codegen artifacts, install pods, and build the New Architecture application
with deployment target 15.1. Ordinary playback remains `expo-video`; this
package is the low-level decode/frame/media-computation capability.

## Linux validation completed

- `npm install --ignore-scripts` completed with no reported root audit
  vulnerabilities.
- `npm run typecheck` passed.
- `npm run prepare` passed and produced CommonJS, ESM, and declaration output.
- RN 0.83.10 library codegen for both platforms passed with
  `generate-codegen-artifacts.js -t all -s library`.
- RN 0.83.10 Android `assembleDebug` passed generated Java, the
  `BindingsInstallerHolder` JNI adapter, common C++, and AAR packaging for the
  requested x86_64 validation ABI. A clean validation used JDK 17.
- The RN 0.64.3 example Android
  `:reactnativenativevideo:clean :reactnativenativevideo:assembleDebug` passed
  the legacy Java module, JNI installer, common C++, and AAR packaging for the
  requested x86_64 validation ABI with JDK 11.
- `npm pack --dry-run --json` passed and included both architecture source sets,
  codegen spec, C++ core, iOS sources, built JS/declarations, and this handoff.

Linux cannot run CocoaPods, Xcode, an iOS Simulator, or a physical decoder
runtime check. Ruby is also unavailable on this host, so podspec parsing was
limited to source inspection. Required Mac/device follow-up is:

1. Run pod install and build the preserved RN 0.64.3 example on the owner's
   Monterey-compatible toolchain.
2. Exercise a real local video: metadata, exact timestamps/index lookup,
   individual and batch decode, RGBA8 bytes, base64/MD5, native preview,
   repeated close, reload, and teardown.
3. Add the package to RetroZero, regenerate Expo SDK 55 native projects/codegen,
   pod install, and build the RN 0.83.10 New Architecture iOS 15.1 target.
4. Repeat the lifecycle checks on iOS and Android hardware, including a
   variable-frame-rate asset and a `content://` source on Android.

No push, npm publish, or release was performed.
