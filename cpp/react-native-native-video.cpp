#include "react-native-native-video.h"

#include <atomic>
#include <cmath>
#include <iterator>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <utility>

using namespace facebook;

namespace SKRNNativeVideo {
namespace {

constexpr const char *kOpenVideoGlobal = "SKRNNativeVideoOpenVideo";

std::atomic<uint64_t> nextNativeFrameId{1};
std::mutex nativeFrameRegistryMutex;
std::unordered_map<
    std::string,
    std::weak_ptr<SKNativeFrameWrapper>> nativeFrameRegistry;

std::string createNativeFrameId() {
  return "native-frame-" + std::to_string(nextNativeFrameId.fetch_add(1));
}

void registerNativeFrame(
    const std::string &nativeId,
    const std::shared_ptr<SKNativeFrameWrapper> &frame) {
  std::lock_guard<std::mutex> lock(nativeFrameRegistryMutex);
  nativeFrameRegistry[nativeId] = frame;
}

void unregisterNativeFrame(const std::string &nativeId) {
  std::lock_guard<std::mutex> lock(nativeFrameRegistryMutex);
  nativeFrameRegistry.erase(nativeId);
}

void requireArgumentCount(
    jsi::Runtime &runtime,
    const char *method,
    size_t actual,
    size_t expected) {
  if (actual < expected) {
    throw jsi::JSError(
        runtime,
        std::string(method) + " requires " + std::to_string(expected) +
            " argument(s)");
  }
}

double requireFiniteNumber(
    jsi::Runtime &runtime,
    const jsi::Value &value,
    const char *label) {
  if (!value.isNumber()) {
    throw jsi::JSError(runtime, std::string(label) + " must be a number");
  }
  const double number = value.asNumber();
  if (!std::isfinite(number)) {
    throw jsi::JSError(runtime, std::string(label) + " must be finite");
  }
  return number;
}

int requireInteger(
    jsi::Runtime &runtime,
    const jsi::Value &value,
    const char *label) {
  const double number = requireFiniteNumber(runtime, value, label);
  if (std::floor(number) != number ||
      number < static_cast<double>(std::numeric_limits<int>::min()) ||
      number > static_cast<double>(std::numeric_limits<int>::max())) {
    throw jsi::JSError(runtime, std::string(label) + " must be an integer");
  }
  return static_cast<int>(number);
}

void requireValid(
    jsi::Runtime &runtime,
    bool isValid,
    const char *resourceName) {
  if (!isValid) {
    throw jsi::JSError(
        runtime, std::string(resourceName) + " is closed or failed to open");
  }
}

} // namespace

SKNativeVideoWrapper::SKNativeVideoWrapper(std::string uri)
    : sourceUri(std::move(uri)) {}

SKNativeFrameWrapper::SKNativeFrameWrapper(int index, double timestamp)
    : index_(index),
      timestamp_(std::isfinite(timestamp) ? timestamp : 0),
      nativeId_(createNativeFrameId()) {}

SKNativeFrameWrapper::~SKNativeFrameWrapper() {
  unregisterNativeFrame(nativeId_);
}

std::shared_ptr<SKNativeFrameWrapper> resolveNativeFrame(
    const std::string &nativeId) {
  std::lock_guard<std::mutex> lock(nativeFrameRegistryMutex);
  const auto entry = nativeFrameRegistry.find(nativeId);
  if (entry == nativeFrameRegistry.end()) {
    return nullptr;
  }
  auto frame = entry->second.lock();
  if (!frame || !frame->isValid()) {
    nativeFrameRegistry.erase(entry);
    return nullptr;
  }
  return frame;
}


struct NativeBufferLease {
  std::shared_ptr<SKNativeFrameWrapper> frame;
  void *buffer = nullptr;
  std::string type;
};

extern "C" SKRNNATIVEVIDEO_BRIDGE_EXPORT void *
SKRNNativeVideoAcquireNativeBufferLease(const char *nativeId) {
  if (nativeId == nullptr) {
    return nullptr;
  }
  auto frame = resolveNativeFrame(nativeId);
  if (!frame || !frame->isValid()) {
    return nullptr;
  }
  void *buffer = frame->nativeBufferPointer();
  if (buffer == nullptr) {
    return nullptr;
  }
  auto *lease = new NativeBufferLease{
      std::move(frame),
      buffer,
      {}};
  lease->type = lease->frame->nativeBufferType();
  return lease;
}

extern "C" SKRNNATIVEVIDEO_BRIDGE_EXPORT void *
SKRNNativeVideoNativeBufferLeaseGetPointer(void *opaqueLease) {
  auto *lease = static_cast<NativeBufferLease *>(opaqueLease);
  return lease == nullptr ? nullptr : lease->buffer;
}

extern "C" SKRNNATIVEVIDEO_BRIDGE_EXPORT const char *
SKRNNativeVideoNativeBufferLeaseGetType(void *opaqueLease) {
  auto *lease = static_cast<NativeBufferLease *>(opaqueLease);
  return lease == nullptr ? nullptr : lease->type.c_str();
}

extern "C" SKRNNATIVEVIDEO_BRIDGE_EXPORT void
SKRNNativeVideoReleaseNativeBufferLease(void *opaqueLease) {
  delete static_cast<NativeBufferLease *>(opaqueLease);
}

jsi::Value SKNativeVideoWrapper::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &name) {
  const std::string property = name.utf8(runtime);

  if (property == "numFrames") {
    return jsi::Value(numFrames());
  }
  if (property == "frameRate") {
    return jsi::Value(frameRate());
  }
  if (property == "size") {
    return ObjectFromSKRNSize(runtime, size());
  }
  if (property == "isValid") {
    return jsi::Value(valid_);
  }
  if (property == "duration") {
    return jsi::Value(duration());
  }
  if (property == "sourceUri") {
    return jsi::String::createFromUtf8(runtime, sourceUri);
  }

  if (property == "getFrameTimestampAtIndex") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        1,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeVideo");
          requireArgumentCount(rt, "getFrameTimestampAtIndex", count, 1);
          const int index = requireInteger(rt, arguments[0], "index");
          if (index < 0 || index >= self->numFrames()) {
            throw jsi::JSError(rt, "index is outside the video frame range");
          }
          return jsi::Value(self->frameTimestampAtIndex(index));
        });
  }

  if (property == "getFrameIndexAtTime") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        1,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeVideo");
          requireArgumentCount(rt, "getFrameIndexAtTime", count, 1);
          const double time = requireFiniteNumber(rt, arguments[0], "time");
          if (time < 0) {
            throw jsi::JSError(rt, "time must be non-negative");
          }
          return jsi::Value(self->frameIndexAtTime(time));
        });
  }

  if (property == "getFrameAtIndex") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        1,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeVideo");
          requireArgumentCount(rt, "getFrameAtIndex", count, 1);
          const int index = requireInteger(rt, arguments[0], "index");
          auto frame = self->getFrameAtIndex(index);
          if (!frame || !frame->isValid()) {
            throw jsi::JSError(
                rt, "Failed to decode frame at index " + std::to_string(index));
          }
          return jsi::Object::createFromHostObject(rt, std::move(frame));
        });
  }

  if (property == "getFramesAtIndex") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        2,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeVideo");
          requireArgumentCount(rt, "getFramesAtIndex", count, 2);
          const int index = requireInteger(rt, arguments[0], "index");
          const int length = requireInteger(rt, arguments[1], "length");
          if (length < 0) {
            throw jsi::JSError(rt, "length must be non-negative");
          }
          return ArrayFromHostObjects(
              rt, self->getFramesAtIndex(index, length));
        });
  }

  if (property == "getFrameAtTime") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        1,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeVideo");
          requireArgumentCount(rt, "getFrameAtTime", count, 1);
          const double time = requireFiniteNumber(rt, arguments[0], "time");
          if (time < 0) {
            throw jsi::JSError(rt, "time must be non-negative");
          }
          auto frame = self->getFrameAtTime(time);
          if (!frame || !frame->isValid()) {
            throw jsi::JSError(
                rt, "Failed to decode frame at time " + std::to_string(time));
          }
          return jsi::Object::createFromHostObject(rt, std::move(frame));
        });
  }

  if (property == "close") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        0,
        [self = std::move(self)](
            jsi::Runtime &,
            const jsi::Value &,
            const jsi::Value *,
            size_t) -> jsi::Value {
          self->close();
          return jsi::Value::undefined();
        });
  }

  return jsi::Value::undefined();
}

std::vector<jsi::PropNameID> SKNativeVideoWrapper::getPropertyNames(
    jsi::Runtime &runtime) {
  static constexpr const char *keys[] = {
      "numFrames",
      "frameRate",
      "size",
      "getFrameAtIndex",
      "getFramesAtIndex",
      "getFrameAtTime",
      "isValid",
      "duration",
      "sourceUri",
      "getFrameTimestampAtIndex",
      "getFrameIndexAtTime",
      "close",
  };
  std::vector<jsi::PropNameID> result;
  result.reserve(std::size(keys));
  for (const char *key : keys) {
    result.push_back(jsi::PropNameID::forAscii(runtime, key));
  }
  return result;
}

jsi::Value SKNativeFrameWrapper::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &name) {
  const std::string property = name.utf8(runtime);

  if (property == "size") {
    return ObjectFromSKRNSize(runtime, size());
  }
  if (property == "bytesPerRow") {
    return jsi::Value(static_cast<double>(bytesPerRow()));
  }
  if (property == "pixelFormat") {
    return jsi::String::createFromAscii(runtime, "rgba8");
  }
  if (property == "platform") {
    return jsi::String::createFromUtf8(runtime, platform());
  }
  if (property == "nativeBufferType") {
    return jsi::String::createFromUtf8(runtime, nativeBufferType());
  }
  if (property == "isValid") {
    return jsi::Value(valid_);
  }
  if (property == "index") {
    return jsi::Value(index_);
  }
  if (property == "timestamp") {
    return jsi::Value(timestamp_);
  }
  if (property == "nativeId" || property == "nativePtrStr") {
    registerNativeFrame(nativeId_, shared_from_this());
    return jsi::String::createFromUtf8(runtime, nativeId_);
  }

  if (property == "arrayBuffer") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        0,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *,
            size_t) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeFrame");
          return self->arrayBufferValue(rt);
        });
  }

  if (property == "close") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        0,
        [self = std::move(self)](
            jsi::Runtime &,
            const jsi::Value &,
            const jsi::Value *,
            size_t) -> jsi::Value {
          self->close();
          return jsi::Value::undefined();
        });
  }

  if (property == "base64") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        1,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeFrame");
          std::string format = "png";
          if (count > 0 && !arguments[0].isUndefined()) {
            if (!arguments[0].isObject()) {
              throw jsi::JSError(rt, "base64 options must be an object");
            }
            const auto value =
                arguments[0].asObject(rt).getProperty(rt, "format");
            if (value.isString()) {
              format = value.asString(rt).utf8(rt);
            }
          }
          return jsi::String::createFromUtf8(rt, self->base64(format));
        });
  }

  if (property == "md5") {
    auto self = shared_from_this();
    return jsi::Function::createFromHostFunction(
        runtime,
        name,
        0,
        [self = std::move(self)](
            jsi::Runtime &rt,
            const jsi::Value &,
            const jsi::Value *,
            size_t) -> jsi::Value {
          requireValid(rt, self->isValid(), "NativeFrame");
          return jsi::String::createFromUtf8(rt, self->md5());
        });
  }

  return jsi::Value::undefined();
}

std::vector<jsi::PropNameID> SKNativeFrameWrapper::getPropertyNames(
    jsi::Runtime &runtime) {
  static constexpr const char *keys[] = {
      "arrayBuffer",
      "size",
      "bytesPerRow",
      "pixelFormat",
      "platform",
      "nativeBufferType",
      "isValid",
      "index",
      "timestamp",
      "nativeId",
      "nativePtrStr",
      "close",
      "base64",
      "md5",
  };
  std::vector<jsi::PropNameID> result;
  result.reserve(std::size(keys));
  for (const char *key : keys) {
    result.push_back(jsi::PropNameID::forAscii(runtime, key));
  }
  return result;
}

void install(jsi::Runtime &runtime, VideoConstructor videoConstructor) {
  auto openVideo = jsi::Function::createFromHostFunction(
      runtime,
      jsi::PropNameID::forAscii(runtime, kOpenVideoGlobal),
      1,
      [videoConstructor = std::move(videoConstructor)](
          jsi::Runtime &rt,
          const jsi::Value &,
          const jsi::Value *arguments,
          size_t count) -> jsi::Value {
        requireArgumentCount(rt, "openVideo", count, 1);
        if (!arguments[0].isString()) {
          throw jsi::JSError(rt, "openVideo uri must be a string");
        }
        const std::string uri = arguments[0].asString(rt).utf8(rt);
        if (uri.empty()) {
          throw jsi::JSError(rt, "openVideo uri must not be empty");
        }
        auto video = videoConstructor(rt, uri);
        if (!video || !video->isValid()) {
          throw jsi::JSError(rt, "Unable to open video: " + uri);
        }
        return jsi::Object::createFromHostObject(rt, std::move(video));
      });

  runtime.global().setProperty(runtime, kOpenVideoGlobal, std::move(openVideo));
}

jsi::Object ObjectFromSKRNSize(jsi::Runtime &runtime, SKRNSize size) {
  jsi::Object object(runtime);
  object.setProperty(runtime, "width", size.width);
  object.setProperty(runtime, "height", size.height);
  return object;
}

} // namespace SKRNNativeVideo
