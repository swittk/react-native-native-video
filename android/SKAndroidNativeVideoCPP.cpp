#include "SKAndroidNativeVideoCPP.h"

#include <android/bitmap.h>
#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>
#include <dlfcn.h>

#include <cstring>
#include <utility>

using namespace facebook;

namespace SKRNNativeVideo {
namespace {

jclass nativeVideoModuleClass = nullptr;
jmethodID createVideoWrapperMethod = nullptr;

jclass videoWrapperClass = nullptr;
jclass frameWrapperClass = nullptr;
jmethodID getFrameAtIndexMethod = nullptr;
jmethodID getFramesAtIndexMethod = nullptr;
jmethodID getNumFramesMethod = nullptr;
jmethodID getFrameRateMethod = nullptr;
jmethodID getDurationMethod = nullptr;
jmethodID getWidthMethod = nullptr;
jmethodID getHeightMethod = nullptr;
jmethodID getFrameTimestampAtIndexMethod = nullptr;
jmethodID getFrameIndexAtTimeMethod = nullptr;
jmethodID closeVideoMethod = nullptr;
jmethodID base64ForBitmapMethod = nullptr;

jmethodID frameGetBitmapMethod = nullptr;
jmethodID frameGetHardwareBufferMethod = nullptr;
jmethodID frameGetWidthMethod = nullptr;
jmethodID frameGetHeightMethod = nullptr;
jmethodID frameCloseMethod = nullptr;

using HardwareBufferFromJavaFn = AHardwareBuffer *(*)(JNIEnv *, jobject);
using HardwareBufferAcquireFn = void (*)(AHardwareBuffer *);
using HardwareBufferReleaseFn = void (*)(AHardwareBuffer *);

HardwareBufferFromJavaFn hardwareBufferFromJava = nullptr;
HardwareBufferAcquireFn hardwareBufferAcquire = nullptr;
HardwareBufferReleaseFn hardwareBufferRelease = nullptr;

void initializeHardwareBufferBindings() {
  void *libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
  if (libandroid == nullptr) {
    return;
  }
  hardwareBufferFromJava = reinterpret_cast<HardwareBufferFromJavaFn>(
      dlsym(libandroid, "AHardwareBuffer_fromHardwareBuffer"));
  hardwareBufferAcquire = reinterpret_cast<HardwareBufferAcquireFn>(
      dlsym(libandroid, "AHardwareBuffer_acquire"));
  hardwareBufferRelease = reinterpret_cast<HardwareBufferReleaseFn>(
      dlsym(libandroid, "AHardwareBuffer_release"));
}

jclass listClass = nullptr;
jmethodID listSizeMethod = nullptr;
jmethodID listGetMethod = nullptr;

class JniEnvironment final {
 public:
  explicit JniEnvironment(JavaVM *jvm) : jvm_(jvm) {
    const jint result =
        jvm_->GetEnv(reinterpret_cast<void **>(&env_), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED &&
        jvm_->AttachCurrentThread(&env_, nullptr) == JNI_OK) {
      attached_ = true;
    }
  }

  ~JniEnvironment() {
    if (attached_) {
      jvm_->DetachCurrentThread();
    }
  }

  JNIEnv *get() const { return env_; }
  explicit operator bool() const { return env_ != nullptr; }

 private:
  JavaVM *jvm_ = nullptr;
  JNIEnv *env_ = nullptr;
  bool attached_ = false;
};

bool clearPendingException(JNIEnv *env) {
  if (!env->ExceptionCheck()) {
    return false;
  }
  env->ExceptionClear();
  return true;
}

jclass globalClass(JNIEnv *env, const char *name) {
  jclass local = env->FindClass(name);
  if (local == nullptr) {
    clearPendingException(env);
    return nullptr;
  }
  jclass global = static_cast<jclass>(env->NewGlobalRef(local));
  env->DeleteLocalRef(local);
  return global;
}

std::string stringFromJString(JNIEnv *env, jstring value) {
  if (value == nullptr) {
    return {};
  }
  const char *characters = env->GetStringUTFChars(value, nullptr);
  if (characters == nullptr) {
    clearPendingException(env);
    return {};
  }
  std::string result(characters);
  env->ReleaseStringUTFChars(value, characters);
  return result;
}

} // namespace

void initializeJavaBindings(JNIEnv *env) {
  nativeVideoModuleClass =
      globalClass(env, "com/reactnativenativevideo/NativeVideoModule");
  videoWrapperClass = globalClass(
      env, "com/reactnativenativevideo/SKNativeVideoWrapperJavaSide");
  frameWrapperClass = globalClass(
      env, "com/reactnativenativevideo/SKAndroidNativeFrameJavaSide");
  listClass = globalClass(env, "java/util/List");
  initializeHardwareBufferBindings();

  createVideoWrapperMethod = env->GetMethodID(
      nativeVideoModuleClass,
      "createVideoWrapper",
      "(Ljava/lang/String;)Lcom/reactnativenativevideo/SKNativeVideoWrapperJavaSide;");
  getFrameAtIndexMethod = env->GetMethodID(
      videoWrapperClass,
      "getFrameAtIndex",
      "(I)Lcom/reactnativenativevideo/SKAndroidNativeFrameJavaSide;");
  getFramesAtIndexMethod = env->GetMethodID(
      videoWrapperClass, "getFramesAtIndex", "(II)Ljava/util/List;");
  getNumFramesMethod =
      env->GetMethodID(videoWrapperClass, "getNumFrames", "()I");
  getFrameRateMethod =
      env->GetMethodID(videoWrapperClass, "getFrameRate", "()D");
  getDurationMethod =
      env->GetMethodID(videoWrapperClass, "getDuration", "()D");
  getWidthMethod = env->GetMethodID(videoWrapperClass, "getWidth", "()I");
  getHeightMethod = env->GetMethodID(videoWrapperClass, "getHeight", "()I");
  getFrameTimestampAtIndexMethod = env->GetMethodID(
      videoWrapperClass, "getFrameTimestampAtIndex", "(I)D");
  getFrameIndexAtTimeMethod = env->GetMethodID(
      videoWrapperClass, "getFrameIndexAtTime", "(D)I");
  closeVideoMethod = env->GetMethodID(videoWrapperClass, "close", "()V");
  base64ForBitmapMethod = env->GetStaticMethodID(
      videoWrapperClass,
      "base64StringForBitmap",
      "(Landroid/graphics/Bitmap;Ljava/lang/String;)Ljava/lang/String;");

  frameGetBitmapMethod =
      env->GetMethodID(frameWrapperClass, "getBitmap", "()Landroid/graphics/Bitmap;");
  frameGetHardwareBufferMethod = env->GetMethodID(
      frameWrapperClass, "getHardwareBuffer", "()Ljava/lang/Object;");
  frameGetWidthMethod =
      env->GetMethodID(frameWrapperClass, "getWidth", "()I");
  frameGetHeightMethod =
      env->GetMethodID(frameWrapperClass, "getHeight", "()I");
  frameCloseMethod =
      env->GetMethodID(frameWrapperClass, "close", "()V");

  listSizeMethod = env->GetMethodID(listClass, "size", "()I");
  listGetMethod =
      env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
}

AndroidModuleState::AndroidModuleState(
    JNIEnv *env,
    jobject nativeVideoModule) {
  env->GetJavaVM(&jvm);
  module = env->NewGlobalRef(nativeVideoModule);
}

AndroidModuleState::~AndroidModuleState() {
  if (jvm == nullptr || module == nullptr) {
    return;
  }
  JniEnvironment environment(jvm);
  if (environment) {
    environment.get()->DeleteGlobalRef(module);
  }
  module = nullptr;
}

SKAndroidNativeVideoWrapper::SKAndroidNativeVideoWrapper(
    const std::string &sourceUri,
    std::shared_ptr<AndroidModuleState> moduleState)
    : SKNativeVideoWrapper(sourceUri), moduleState_(std::move(moduleState)) {
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return;
  }
  JNIEnv *env = environment.get();
  jstring uri = env->NewStringUTF(sourceUri.c_str());
  jobject localWrapper = env->CallObjectMethod(
      moduleState_->module, createVideoWrapperMethod, uri);
  env->DeleteLocalRef(uri);
  if (clearPendingException(env) || localWrapper == nullptr) {
    return;
  }
  javaVideoWrapper_ = env->NewGlobalRef(localWrapper);
  env->DeleteLocalRef(localWrapper);
  setValid(javaVideoWrapper_ != nullptr);
}

SKAndroidNativeVideoWrapper::~SKAndroidNativeVideoWrapper() {
  close();
}

void SKAndroidNativeVideoWrapper::close() {
  if (javaVideoWrapper_ == nullptr || moduleState_ == nullptr) {
    setValid(false);
    return;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (environment) {
    JNIEnv *env = environment.get();
    env->CallVoidMethod(javaVideoWrapper_, closeVideoMethod);
    clearPendingException(env);
    env->DeleteGlobalRef(javaVideoWrapper_);
  }
  javaVideoWrapper_ = nullptr;
  setValid(false);
}

std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtIndex(int index) {
  const int frameCount = numFrames();
  if (!valid_ || index < 0 || index >= frameCount) {
    return nullptr;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return nullptr;
  }
  JNIEnv *env = environment.get();
  jobject javaFrame =
      env->CallObjectMethod(javaVideoWrapper_, getFrameAtIndexMethod, index);
  if (clearPendingException(env) || javaFrame == nullptr) {
    return nullptr;
  }
  auto result = std::make_shared<SKAndroidNativeFrameWrapper>(
      moduleState_->jvm,
      env,
      javaFrame,
      index,
      frameTimestampAtIndex(index));
  env->DeleteLocalRef(javaFrame);
  return result;
}

std::vector<std::shared_ptr<SKNativeFrameWrapper>>
SKAndroidNativeVideoWrapper::getFramesAtIndex(int index, int length) {
  std::vector<std::shared_ptr<SKNativeFrameWrapper>> result;
  const int frameCount = numFrames();
  if (!valid_ || index < 0 || index >= frameCount || length <= 0) {
    return result;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return result;
  }
  JNIEnv *env = environment.get();
  jobject list = env->CallObjectMethod(
      javaVideoWrapper_, getFramesAtIndexMethod, index, length);
  if (clearPendingException(env) || list == nullptr) {
    return result;
  }

  const jint count = env->CallIntMethod(list, listSizeMethod);
  result.reserve(count);
  for (jint item = 0; item < count; ++item) {
    jobject javaFrame = env->CallObjectMethod(list, listGetMethod, item);
    if (!clearPendingException(env) && javaFrame != nullptr) {
      const int frameIndex = index + item;
      auto frame = std::make_shared<SKAndroidNativeFrameWrapper>(
          moduleState_->jvm,
          env,
          javaFrame,
          frameIndex,
          frameTimestampAtIndex(frameIndex));
      if (frame->isValid()) {
        result.push_back(std::move(frame));
      }
    }
    if (javaFrame != nullptr) {
      env->DeleteLocalRef(javaFrame);
    }
  }
  env->DeleteLocalRef(list);
  return result;
}

std::shared_ptr<SKNativeFrameWrapper>
SKAndroidNativeVideoWrapper::getFrameAtTime(double time) {
  const int frameIndex = frameIndexAtTime(time);
  if (frameIndex < 0) {
    return nullptr;
  }
  // Decode the frame selected by the same PTS/index mapping exposed to JS so
  // the returned frame's index and timestamp cannot disagree with its pixels.
  return getFrameAtIndex(frameIndex);
}

int SKAndroidNativeVideoWrapper::numFrames() const {
  if (!valid_) {
    return 0;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return 0;
  }
  JNIEnv *env = environment.get();
  const jint value = env->CallIntMethod(javaVideoWrapper_, getNumFramesMethod);
  return clearPendingException(env) ? 0 : value;
}

double SKAndroidNativeVideoWrapper::frameRate() const {
  if (!valid_) {
    return 0;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return 0;
  }
  JNIEnv *env = environment.get();
  const jdouble value =
      env->CallDoubleMethod(javaVideoWrapper_, getFrameRateMethod);
  return clearPendingException(env) ? 0 : value;
}

SKRNSize SKAndroidNativeVideoWrapper::size() const {
  if (!valid_) {
    return {0, 0};
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return {0, 0};
  }
  JNIEnv *env = environment.get();
  const jint width = env->CallIntMethod(javaVideoWrapper_, getWidthMethod);
  const jint height = env->CallIntMethod(javaVideoWrapper_, getHeightMethod);
  return clearPendingException(env)
      ? SKRNSize{0, 0}
      : SKRNSize{static_cast<double>(width), static_cast<double>(height)};
}

double SKAndroidNativeVideoWrapper::duration() const {
  if (!valid_) {
    return 0;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return 0;
  }
  JNIEnv *env = environment.get();
  const jdouble value =
      env->CallDoubleMethod(javaVideoWrapper_, getDurationMethod);
  return clearPendingException(env) ? 0 : value;
}

double SKAndroidNativeVideoWrapper::frameTimestampAtIndex(int index) const {
  if (!valid_ || index < 0) {
    return 0;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return 0;
  }
  JNIEnv *env = environment.get();
  const jdouble value = env->CallDoubleMethod(
      javaVideoWrapper_, getFrameTimestampAtIndexMethod, index);
  return clearPendingException(env) ? 0 : value;
}

int SKAndroidNativeVideoWrapper::frameIndexAtTime(double time) const {
  if (!valid_ || time < 0) {
    return -1;
  }
  JniEnvironment environment(moduleState_->jvm);
  if (!environment) {
    return -1;
  }
  JNIEnv *env = environment.get();
  const jint value = env->CallIntMethod(
      javaVideoWrapper_, getFrameIndexAtTimeMethod, time);
  return clearPendingException(env) ? -1 : value;
}

SKAndroidNativeFrameWrapper::SKAndroidNativeFrameWrapper(
    JavaVM *jvm,
    JNIEnv *env,
    jobject javaFrame,
    int frameIndex,
    double frameTimestamp)
    : SKNativeFrameWrapper(frameIndex, frameTimestamp), jvm_(jvm) {
  if (javaFrame == nullptr) {
    return;
  }

  javaFrame_ = env->NewGlobalRef(javaFrame);
  if (javaFrame_ == nullptr) {
    return;
  }

  if (hardwareBufferFromJava != nullptr &&
      hardwareBufferAcquire != nullptr &&
      frameGetHardwareBufferMethod != nullptr) {
    jobject javaHardwareBuffer =
        env->CallObjectMethod(javaFrame_, frameGetHardwareBufferMethod);
    if (!clearPendingException(env) && javaHardwareBuffer != nullptr) {
      AHardwareBuffer *buffer =
          hardwareBufferFromJava(env, javaHardwareBuffer);
      if (buffer != nullptr) {
        hardwareBufferAcquire(buffer);
        hardwareBuffer_ = buffer;
      }
      env->DeleteLocalRef(javaHardwareBuffer);
    }
  }


  setValid(true);
}

SKAndroidNativeFrameWrapper::~SKAndroidNativeFrameWrapper() {
  close();
}

std::string SKAndroidNativeFrameWrapper::nativeBufferType() const {
  std::lock_guard<std::mutex> lock(frameMutex_);
  return hardwareBuffer_ != nullptr ? "hardwareBuffer" : "bitmap";
}

void *SKAndroidNativeFrameWrapper::nativeBufferPointer() const {
  std::lock_guard<std::mutex> lock(frameMutex_);
  return valid_ ? static_cast<void *>(hardwareBuffer_) : nullptr;
}

void *SKAndroidNativeFrameWrapper::retainNativeBufferPointer() const {
  std::lock_guard<std::mutex> lock(frameMutex_);
  if (!valid_ || hardwareBuffer_ == nullptr || hardwareBufferAcquire == nullptr) {
    return nullptr;
  }
  hardwareBufferAcquire(hardwareBuffer_);
  return static_cast<void *>(hardwareBuffer_);
}

void SKAndroidNativeFrameWrapper::releaseNativeBufferPointer(
    void *buffer) const {
  if (buffer != nullptr && hardwareBufferRelease != nullptr) {
    hardwareBufferRelease(static_cast<AHardwareBuffer *>(buffer));
  }
}

void SKAndroidNativeFrameWrapper::close() {
  std::lock_guard<std::mutex> lock(frameMutex_);
  if (!valid_ && javaFrame_ == nullptr && hardwareBuffer_ == nullptr) {
    return;
  }

  if (hardwareBuffer_ != nullptr && hardwareBufferRelease != nullptr) {
    hardwareBufferRelease(hardwareBuffer_);
    hardwareBuffer_ = nullptr;
  }

  if (javaFrame_ != nullptr && jvm_ != nullptr) {
    JniEnvironment environment(jvm_);
    if (environment) {
      JNIEnv *env = environment.get();
      if (frameCloseMethod != nullptr) {
        env->CallVoidMethod(javaFrame_, frameCloseMethod);
        clearPendingException(env);
      }
      env->DeleteGlobalRef(javaFrame_);
    }
    javaFrame_ = nullptr;
  }
  setValid(false);
}

jobject SKAndroidNativeFrameWrapper::bitmap(JNIEnv *env) const {
  std::lock_guard<std::mutex> lock(frameMutex_);
  if (!valid_ || javaFrame_ == nullptr || frameGetBitmapMethod == nullptr) {
    return nullptr;
  }
  jobject result = env->CallObjectMethod(javaFrame_, frameGetBitmapMethod);
  if (clearPendingException(env)) {
    return nullptr;
  }
  return result;
}

SKRNSize SKAndroidNativeFrameWrapper::size() const {
  std::lock_guard<std::mutex> lock(frameMutex_);
  if (!valid_ || javaFrame_ == nullptr) {
    return {0, 0};
  }
  JniEnvironment environment(jvm_);
  if (!environment) {
    return {0, 0};
  }
  JNIEnv *env = environment.get();
  const jint width = env->CallIntMethod(javaFrame_, frameGetWidthMethod);
  const jint height = env->CallIntMethod(javaFrame_, frameGetHeightMethod);
  if (clearPendingException(env)) {
    return {0, 0};
  }
  return {
      static_cast<double>(width), static_cast<double>(height)};
}

size_t SKAndroidNativeFrameWrapper::bytesPerRow() const {
  return static_cast<size_t>(size().width) * 4;
}

std::string SKAndroidNativeFrameWrapper::base64(const std::string &format) {
  JniEnvironment environment(jvm_);
  if (!environment) {
    return {};
  }
  JNIEnv *env = environment.get();
  jobject frameBitmap = bitmap(env);
  if (frameBitmap == nullptr) {
    return {};
  }
  jstring javaFormat = env->NewStringUTF(format.c_str());
  jstring encoded = static_cast<jstring>(env->CallStaticObjectMethod(
      videoWrapperClass,
      base64ForBitmapMethod,
      frameBitmap,
      javaFormat));
  env->DeleteLocalRef(javaFormat);
  env->DeleteLocalRef(frameBitmap);
  if (clearPendingException(env) || encoded == nullptr) {
    return {};
  }
  std::string result = stringFromJString(env, encoded);
  env->DeleteLocalRef(encoded);
  return result;
}

jsi::Value SKAndroidNativeFrameWrapper::arrayBufferValue(
    jsi::Runtime &runtime) {
  JniEnvironment environment(jvm_);
  if (!environment) {
    throw jsi::JSError(runtime, "Unable to attach to the Android runtime");
  }
  JNIEnv *env = environment.get();
  jobject frameBitmap = bitmap(env);
  if (frameBitmap == nullptr) {
    throw jsi::JSError(runtime, "Unable to rasterize Android frame");
  }

  AndroidBitmapInfo info{};
  if (AndroidBitmap_getInfo(env, frameBitmap, &info) !=
          ANDROID_BITMAP_RESULT_SUCCESS ||
      info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    env->DeleteLocalRef(frameBitmap);
    throw jsi::JSError(runtime, "Android frame is not an RGBA8 bitmap");
  }

  const size_t rowBytes = static_cast<size_t>(info.width) * 4;
  const size_t totalBytes = rowBytes * info.height;
  jsi::Function constructor =
      runtime.global().getPropertyAsFunction(runtime, "ArrayBuffer");
  jsi::Object object = constructor
                           .callAsConstructor(
                               runtime,
                               jsi::Value(static_cast<double>(totalBytes)))
                           .getObject(runtime);
  uint8_t *destination = object.getArrayBuffer(runtime).data(runtime);

  void *pixels = nullptr;
  if (AndroidBitmap_lockPixels(env, frameBitmap, &pixels) !=
          ANDROID_BITMAP_RESULT_SUCCESS ||
      pixels == nullptr) {
    env->DeleteLocalRef(frameBitmap);
    throw jsi::JSError(runtime, "Unable to lock Android frame pixels");
  }
  const auto *source = static_cast<const uint8_t *>(pixels);
  for (uint32_t row = 0; row < info.height; ++row) {
    memcpy(destination + row * rowBytes, source + row * info.stride, rowBytes);
  }
  AndroidBitmap_unlockPixels(env, frameBitmap);
  env->DeleteLocalRef(frameBitmap);
  return object;
}

} // namespace SKRNNativeVideo

struct AndroidFrameHolder {
  jobject bitmap = nullptr;
};

extern "C" JNIEXPORT jlong JNICALL
Java_com_reactnativenativevideo_SKRNNativeFrameViewManager_acquireNativeFrame(
    JNIEnv *env,
    jclass,
    jstring nativeId) {
  auto frame = std::dynamic_pointer_cast<
      SKRNNativeVideo::SKAndroidNativeFrameWrapper>(
      SKRNNativeVideo::resolveNativeFrame(
          SKRNNativeVideo::stringFromJString(env, nativeId)));
  if (!frame || !frame->isValid()) {
    return 0;
  }
  jobject localBitmap = frame->bitmap(env);
  if (localBitmap == nullptr) {
    return 0;
  }
  jobject globalBitmap = env->NewGlobalRef(localBitmap);
  env->DeleteLocalRef(localBitmap);
  if (globalBitmap == nullptr) {
    return 0;
  }
  return reinterpret_cast<jlong>(new AndroidFrameHolder{globalBitmap});
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_reactnativenativevideo_SKRNNativeFrameViewManager_bitmapForNativeFrame(
    JNIEnv *env,
    jclass,
    jlong nativeFrame) {
  const auto *holder = reinterpret_cast<AndroidFrameHolder *>(nativeFrame);
  return holder == nullptr || holder->bitmap == nullptr
      ? nullptr
      : env->NewLocalRef(holder->bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_reactnativenativevideo_SKRNNativeFrameViewManager_releaseNativeFrame(
    JNIEnv *env,
    jclass,
    jlong nativeFrame) {
  auto *holder = reinterpret_cast<AndroidFrameHolder *>(nativeFrame);
  if (holder != nullptr) {
    if (holder->bitmap != nullptr) {
      env->DeleteGlobalRef(holder->bitmap);
    }
    delete holder;
  }
}
