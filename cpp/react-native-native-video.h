#ifndef REACT_NATIVE_NATIVE_VIDEO_COMMON_HEADER_FILE
#define REACT_NATIVE_NATIVE_VIDEO_COMMON_HEADER_FILE
#include <memory>
#include <jsi/jsi.h>

namespace facebook {
namespace jsi {
class Runtime;
}
namespace react {
class CallInvoker;
}
}

namespace SKRNNativeVideo {
std::string PointerToString(void* cb);
void *StringToPointer(std::string& str);

class SKNativeVideoWrapper;
typedef struct SKRNSize {
    double width;
    double height;
} SKRNSize;
facebook::jsi::Object ObjectFromSKRNSize(facebook::jsi::Runtime &runtime, SKRNSize size);
template <typename JSICompatibleType>
facebook::jsi::Array ArrayFromJSICompatibleVector(facebook::jsi::Runtime &runtime, std::vector<JSICompatibleType> vec) {
    facebook::jsi::Array arr(runtime, vec.size());
    for(int i = 0; i < vec.size(); i++) {
        arr.setValueAtIndex(runtime, i, facebook::jsi::Object::createFromHostObject(runtime, vec[i]));
    }
    return arr;
}

int multiply(float a, float b);

void install(facebook::jsi::Runtime &jsiRuntime, std::function<std::shared_ptr<SKNativeVideoWrapper>(facebook::jsi::Runtime&, std::string)> videoConstructor);
//void install(facebook::jsi::Runtime &jsiRuntime, std::shared_ptr<facebook::react::CallInvoker> invoker);
void cleanup(facebook::jsi::Runtime &jsiRuntime);

// Override this class in each platform's implementation
class SKNativeFrameWrapper : public facebook::jsi::HostObject {
protected:
    bool _valid = false;
    // Be sure to set this to true in subclasses when video is loaded
    void setValid(bool v) {_valid = v;};
public:
    facebook::jsi::Runtime &runtime;
    bool isValid() { return _valid; };
    facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name);
    std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime& rt);
    SKNativeFrameWrapper(facebook::jsi::Runtime &_runtime) : runtime(_runtime) {};
    
    // Make sure to implement these methods!
    /** This is potentially for casting the correct type  (should return "iOS" for iOS and "Android" for Android)*/
    virtual std::string platform() { return "null"; };
    // This should free/close native resources
    virtual void close() {};
    // Supposed to return ArrayBuffer
    virtual facebook::jsi::Value arrayBufferValue() { return facebook::jsi::Value::undefined(); }
    virtual SKRNSize size() {return (SKRNSize){0, 0};}
    virtual std::string base64(std::string format = "") {return std::string();}
    virtual std::string md5() {return std::string();}
};

class SKNativeVideoWrapper : public facebook::jsi::HostObject {
protected:
    bool _valid = false;
    // Be sure to set this to true in subclasses when video is loaded
    void setValid(bool v) {_valid = v;};
public:
    facebook::jsi::Runtime &runtime;
    std::string sourceUri;
    SKNativeVideoWrapper(facebook::jsi::Runtime &runtime, std::string sourceUri);
    facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name);
    std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime& rt);
    
    // Make sure to implement these methods!
    virtual void close() {};
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index) {return std::make_shared<SKNativeFrameWrapper>(runtime); };
    virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(int index, int numFrames) {return std::vector<std::shared_ptr<SKNativeFrameWrapper>>{};};
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time) {return std::make_shared<SKNativeFrameWrapper>(runtime);};
    virtual int numFrames() {return 0;};
    virtual double frameRate() {return 0; };
    virtual SKRNSize size() {return (SKRNSize){0, 0};}
    virtual double duration() {return 0;}
};
}

#endif /* REACT_NATIVE_NATIVE_VIDEO_COMMON_HEADER_FILE */
