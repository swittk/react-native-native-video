#ifndef EXAMPLE_H
#define EXAMPLE_H
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
int multiply(float a, float b);

void install(facebook::jsi::Runtime &jsiRuntime);
//void install(facebook::jsi::Runtime &jsiRuntime, std::shared_ptr<facebook::react::CallInvoker> invoker);
void cleanup(facebook::jsi::Runtime &jsiRuntime);

// Override this class in each platform's implementation
class SKNativeFrameWrapper : public facebook::jsi::HostObject {
protected:
    bool _valid = false;
    // Be sure to set this to true in subclasses when video is loaded
    void setValid(bool v) {_valid = v;};
public:
    bool isValid() { return _valid; };
    facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name);
    std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime& rt);
    
    // Make sure to implement these methods!
    /** This is potentially for casting the correct type  (should return "iOS" for iOS and "Android" for Android)*/
    virtual std::string platform() { return "null"; };
    // This should free/close native resources
    virtual void close() {};
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
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtIndex(int index) {return std::make_shared<SKNativeFrameWrapper>(); };
    virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(int index, int numFrames) {return std::vector<std::shared_ptr<SKNativeFrameWrapper>>{};};
    virtual std::shared_ptr<SKNativeFrameWrapper> getFrameAtTime(double time) {return std::make_shared<SKNativeFrameWrapper>();};
    virtual int numFrames() {return 0;};
    virtual double frameRate() {return 0; };
};
}

#endif /* EXAMPLE_H */
