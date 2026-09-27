#ifndef REACT_NATIVE_NATIVE_VIDEO_COMMON_HEADER_FILE
#define REACT_NATIVE_NATIVE_VIDEO_COMMON_HEADER_FILE

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <jsi/jsi.h>

namespace SKRNNativeVideo {

struct SKRNSize {
  double width;
  double height;
};

class SKNativeVideoWrapper;
class SKNativeFrameWrapper;

using VideoConstructor = std::function<std::shared_ptr<SKNativeVideoWrapper>(
    facebook::jsi::Runtime &,
    const std::string &)>;

facebook::jsi::Object ObjectFromSKRNSize(
    facebook::jsi::Runtime &runtime,
    SKRNSize size);

template <typename HostObjectType>
facebook::jsi::Array ArrayFromHostObjects(
    facebook::jsi::Runtime &runtime,
    const std::vector<std::shared_ptr<HostObjectType>> &values) {
  facebook::jsi::Array array(runtime, values.size());
  for (size_t index = 0; index < values.size(); ++index) {
    array.setValueAtIndex(
        runtime,
        index,
        facebook::jsi::Object::createFromHostObject(runtime, values[index]));
  }
  return array;
}

/** Installs the synchronous HostObject factory into the supplied RN runtime. */
void install(facebook::jsi::Runtime &runtime, VideoConstructor videoConstructor);

/**
 * Resolves the opaque identifier exposed by NativeFrameWrapper. Native preview
 * views use this registry so they retain a shared_ptr instead of dereferencing
 * a serialized raw pointer after the HostObject has been collected.
 */
std::shared_ptr<SKNativeFrameWrapper> resolveNativeFrame(
    const std::string &nativeId);

class SKNativeFrameWrapper
    : public facebook::jsi::HostObject,
      public std::enable_shared_from_this<SKNativeFrameWrapper> {
 protected:
  bool valid_ = false;
  void setValid(bool value) { valid_ = value; }

 public:
  explicit SKNativeFrameWrapper(int index = -1, double timestamp = 0);
  ~SKNativeFrameWrapper() override;

  bool isValid() const { return valid_; }
  int index() const { return index_; }
  double timestamp() const { return timestamp_; }
  const std::string &nativeId() const { return nativeId_; }

  facebook::jsi::Value get(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::PropNameID &name) override;
  std::vector<facebook::jsi::PropNameID> getPropertyNames(
      facebook::jsi::Runtime &runtime) override;

  virtual std::string platform() const { return "unknown"; }
  virtual std::string nativeBufferType() const { return "unknown"; }
  /**
   * Returns the platform-native decoded buffer while this frame is alive.
   * iOS: CVPixelBufferRef. Android fast path: AHardwareBuffer*.
   * The pointer is borrowed; callers must hold a NativeBuffer lease.
   */
  virtual void *nativeBufferPointer() const { return nullptr; }
  virtual void close() {}
  virtual facebook::jsi::Value arrayBufferValue(
      facebook::jsi::Runtime &) {
    return facebook::jsi::Value::undefined();
  }
  virtual SKRNSize size() const { return {0, 0}; }
  virtual size_t bytesPerRow() const {
    return static_cast<size_t>(size().width) * 4;
  }
  virtual std::string base64(const std::string &) { return {}; }
  virtual std::string md5() { return {}; }

 private:
  const int index_;
  const double timestamp_;
  const std::string nativeId_;
};

class SKNativeVideoWrapper
    : public facebook::jsi::HostObject,
      public std::enable_shared_from_this<SKNativeVideoWrapper> {
 protected:
  bool valid_ = false;
  void setValid(bool value) { valid_ = value; }

 public:
  explicit SKNativeVideoWrapper(std::string sourceUri);
  ~SKNativeVideoWrapper() override = default;

  const std::string sourceUri;
  bool isValid() const { return valid_; }

  facebook::jsi::Value get(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::PropNameID &name) override;
  std::vector<facebook::jsi::PropNameID> getPropertyNames(
      facebook::jsi::Runtime &runtime) override;

  virtual void close() {}
  virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index) = 0;
  virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(
      int index,
      int numFrames) = 0;
  virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time) = 0;
  virtual int numFrames() const = 0;
  virtual double frameRate() const = 0;
  virtual SKRNSize size() const = 0;
  virtual double duration() const = 0;
  virtual double frameTimestampAtIndex(int index) const = 0;
  virtual int frameIndexAtTime(double time) const = 0;
};



/**
 * Stable C ABI for companion native adapters. A lease retains the underlying
 * NativeFrame HostObject while a consumer imports its native buffer.
 *
 * The returned native-buffer pointer is borrowed from the lease. Consumers
 * should import/copy/wrap it synchronously, then release the lease.
 */
extern "C" void *SKRNNativeVideoAcquireNativeBufferLease(const char *nativeId);
extern "C" void *SKRNNativeVideoNativeBufferLeaseGetPointer(void *lease);
extern "C" const char *SKRNNativeVideoNativeBufferLeaseGetType(void *lease);
extern "C" void SKRNNativeVideoReleaseNativeBufferLease(void *lease);

} // namespace SKRNNativeVideo

#endif
