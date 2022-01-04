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
        case "numFrames"_sh:{
            return jsi::Value(numFrames());
        } break;
        case "frameRate"_sh: {
            return jsi::Value(frameRate());
        } break;
        case "size"_sh: {
            return ObjectFromSKRNSize(runtime, size());
        } break;
        case "getFrameAtIndex"_sh:{
            return jsi::Function::createFromHostFunction(runtime, name, 1, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                                           {
//                if(count < 1) return jsi::JSError(runtime, "No frame index supplied");
                int idx = arguments[0].asNumber();
                return jsi::Object::createFromHostObject(runtime, getFrameAtIndex(idx));
            });
        } break;
        case "getFramesAtIndex"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 2, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                                           {
//                if(count < 1) return jsi::JSError(runtime, "No frame index supplied");
                int idx = arguments[0].asNumber();
                int len = arguments[1].asNumber();
                auto frames = getFramesAtIndex(idx, len);
                return ArrayFromJSICompatibleVector(runtime, frames);
            });
        } break;
        case "getFrameAtTime"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 1, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                                           {
//                if(count < 1) return jsi::JSError(runtime, "No frame index supplied");
                double time = arguments[0].asNumber();
                return jsi::Object::createFromHostObject(runtime, getFrameAtTime(time));
            });
        } break;
        case "isValid"_sh: {
            return jsi::Value(_valid);
        } break;
        case "duration"_sh: {
            return jsi::Value(duration());
        } break;
        default:
            break;
    }
    return jsi::Value::undefined();
}
static std::vector<std::string> nativeVideoWrapperKeys = {
    "numFrames",
    "frameRate",
    "size",
    "getFrameAtIndex",
    "getFramesAtIndex",
    "getFrameAtTime",
    "isValid",
    "duration"
};

std::vector<jsi::PropNameID> SKNativeVideoWrapper::getPropertyNames(jsi::Runtime& rt) {
    std::vector<jsi::PropNameID> ret;
    for(std::string key : nativeVideoWrapperKeys) {
        ret.push_back(jsi::PropNameID::forUtf8(rt, key));
    }
    return ret;
}


static std::vector<std::string> nativeFrameWrapperKeys = {
    "arrayBuffer",
    "size",
    "isValid",
};
jsi::Value SKNativeFrameWrapper::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
    std::string methodName = name.utf8(runtime);
    long long methodSwitch = string_hash(methodName.c_str());
    switch(methodSwitch) {
        case "arrayBuffer"_sh: {
            return arrayBufferValue();
        } break;
        case "size"_sh: {
            return ObjectFromSKRNSize(runtime, size());
        } break;
        case "isValid"_sh: {
            return jsi::Value(_valid);
        } break;
        default:
        break;
    }
    return jsi::Value::undefined();
}

std::vector<jsi::PropNameID> SKNativeFrameWrapper::getPropertyNames(jsi::Runtime& rt) {
    std::vector<jsi::PropNameID> ret;
    for(std::string key : nativeFrameWrapperKeys) {
        ret.push_back(jsi::PropNameID::forUtf8(rt, key));
    }
    return ret;
}


int multiply(float a, float b) {
    return a * b;
}

// Hopefully I won't need the invoker this time...
//void install(facebook::jsi::Runtime &jsiRuntime, std::shared_ptr<facebook::react::CallInvoker> invoker) {
void install(facebook::jsi::Runtime &jsiRuntime, std::function<std::shared_ptr<SKNativeVideoWrapper>(jsi::Runtime&, std::string)> videoConstructor) {
    auto openVideoFunction =
    jsi::Function::createFromHostFunction(
                                          jsiRuntime,
                                          PropNameID::forAscii(jsiRuntime, "SKRNNativeVideoOpenVideo"),
                                          0,
//                                          [&, invoker](Runtime &runtime, const Value &thisValue, const Value *arguments,
                                          [&, videoConstructor](Runtime &runtime, const Value &thisValue, const Value *arguments,
                                                       size_t count) -> Value
                                          {
                                              if(count < 1) return jsi::Value::undefined();
                                              std::string path = arguments[0].asString(runtime).utf8(runtime);
                                              std::shared_ptr<SKNativeVideoWrapper> obj = videoConstructor(runtime, path);
//                                              auto obj = std::make_shared<SKNativeVideoWrapper>(runtime, path);
                                              jsi::Object object = jsi::Object::createFromHostObject(runtime, obj);
                                              return object;
                                          });
    jsiRuntime.global().setProperty(jsiRuntime, "SKRNNativeVideoOpenVideo",
                                    std::move(openVideoFunction));
}
void cleanup(facebook::jsi::Runtime &jsiRuntime) {
    
}


facebook::jsi::Object ObjectFromSKRNSize(facebook::jsi::Runtime &runtime, SKRNSize size) {
    jsi::Object obj = jsi::Object(runtime);
    obj.setProperty(runtime, "width", size.width);
    obj.setProperty(runtime, "height", size.height);
    return obj;
}
}
