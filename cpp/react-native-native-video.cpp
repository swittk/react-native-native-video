#include "react-native-native-video.h"
#include <jsi/jsi.h>
#include <map>
#include <string>
#include <ReactCommon/CallInvoker.h>
#include "CPPNumericStringHashCompare.h"

using namespace facebook;
using namespace jsi;

namespace SKRNNativeVideo {

SKNativeVideoWrapper::SKNativeVideoWrapper(facebook::jsi::Runtime &rt, std::string uri): runtime(rt), sourceUri(uri) {}
jsi::Value SKNativeVideoWrapper::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
    std::string methodName = name.utf8(runtime);
    long long methodSwitch = string_hash(methodName.c_str());
    switch (methodSwitch) {
        case "nativeFrame"_sh: {
            // return this platform's NativeFrame wrapper
//            return getNativeFrame();
        } break;
        case "buffer"_sh:{
            
        } break;
        case "width"_sh: {
            
        } break;
        case "height"_sh:{
            
        } break;
        case "getFrameAtIndex"_sh:{
            return jsi::Function::createFromHostFunction(runtime, name, 1, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                                           {
//                if(count < 1) return jsi::JSError(runtime, "No frame index supplied");
                int idx = arguments[1].asNumber();
                return jsi::Object::createFromHostObject(runtime, getFrameAtIndex(idx));
            });
        } break;
            
        default:
            break;
    }
    return jsi::Value::undefined();
}
std::vector<jsi::PropNameID> SKNativeVideoWrapper::getPropertyNames(jsi::Runtime& rt) {
    std::vector<jsi::PropNameID> ret;
    ret.push_back(jsi::PropNameID::forAscii(rt, "nativeFrame"));
    // get an arraybuffer wrapping the memory of the frame
    ret.push_back(jsi::PropNameID::forAscii(rt, "buffer"));
    ret.push_back(jsi::PropNameID::forAscii(rt, "width"));
    ret.push_back(jsi::PropNameID::forAscii(rt, "height"));
    return ret;
}

jsi::Value SKNativeFrameWrapper::get(jsi::Runtime &runtime, const jsi::PropNameID &name) { return jsi::Value::undefined(); }

std::vector<jsi::PropNameID> SKNativeFrameWrapper::getPropertyNames(jsi::Runtime& rt) {
    std::vector<jsi::PropNameID> ret;
    // get an arraybuffer wrapping the memory of the frame
    ret.push_back(jsi::PropNameID::forAscii(rt, "buffer"));
    ret.push_back(jsi::PropNameID::forAscii(rt, "width"));
    ret.push_back(jsi::PropNameID::forAscii(rt, "height"));
    ret.push_back(jsi::PropNameID::forAscii(rt, "isValid"));
    return ret;
}


int multiply(float a, float b) {
    return a * b;
}

// Hopefully I won't need the invoker this time...
//void install(facebook::jsi::Runtime &jsiRuntime, std::shared_ptr<facebook::react::CallInvoker> invoker) {
void install(facebook::jsi::Runtime &jsiRuntime) {
    auto openVideoFunction =
    jsi::Function::createFromHostFunction(
                                          jsiRuntime,
                                          PropNameID::forAscii(jsiRuntime, "SKRNNativeVideoOpenVideo"),
                                          0,
//                                          [&, invoker](Runtime &runtime, const Value &thisValue, const Value *arguments,
                                          [&](Runtime &runtime, const Value &thisValue, const Value *arguments,
                                                       size_t count) -> Value
                                          {
                                              if(count < 1) return jsi::Value::undefined();
                                              std::string path = arguments[0].asString(runtime).utf8(runtime);
                                              auto obj = std::make_shared<SKNativeVideoWrapper>(runtime, path);
                                              jsi::Object object = jsi::Object::createFromHostObject(runtime, obj);
                                              return object;
                                          });
    jsiRuntime.global().setProperty(jsiRuntime, "SKRNNativeVideoOpenVideo",
                                    std::move(openVideoFunction));
}
void cleanup(facebook::jsi::Runtime &jsiRuntime) {
    
}
}
