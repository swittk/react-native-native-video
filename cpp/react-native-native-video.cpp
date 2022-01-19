#include "react-native-native-video.h"
#include <jsi/jsi.h>
#include <map>
#include <string>
#include <ReactCommon/CallInvoker.h>
#include "CPPNumericStringHashCompare.h"
#include <sstream>

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
                auto res = getFrameAtIndex(idx);
                if(res.get() == nullptr) {
                    //                    return jsi::Value::undefined();
                    throw jsi::JSError(runtime, "Failed to get frame");
                }
                return jsi::Object::createFromHostObject(runtime, res);
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
                auto res = getFrameAtTime(time);
                if(res.get() == nullptr) {
                    //                    return jsi::Value::undefined();
                    throw jsi::JSError(runtime, "Failed to get frame");
                }
                return jsi::Object::createFromHostObject(runtime, res);
            });
        } break;
        case "isValid"_sh: {
            return jsi::Value(_valid);
        } break;
        case "duration"_sh: {
            return jsi::Value(duration());
        } break;
        case "sourceUri"_sh: {
            return jsi::String::createFromUtf8(runtime, sourceUri);
        } break;
        case "close"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 0, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                         {
                close();
                return jsi::Value::undefined();
            });
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
    "duration",
    "sourceUri",
    "close"
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
    "nativePtrStr",
    "close",
    "base64",
    "png_md5"
};
jsi::Value SKNativeFrameWrapper::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
    std::string methodName = name.utf8(runtime);
    long long methodSwitch = string_hash(methodName.c_str());
    switch(methodSwitch) {
        case "arrayBuffer"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 0, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                         {
                //                if(count < 1) return jsi::JSError(runtime, "No frame index supplied");
                return arrayBufferValue();
            });
            
            //            printf("ArrayBuffer called with hash %llu when methodSwitch is %llu and method is %s", "arrayBuffer"_sh, methodSwitch, methodName.c_str());
        } break;
        case "size"_sh: {
            return ObjectFromSKRNSize(runtime, size());
        } break;
        case "isValid"_sh: {
            return jsi::Value(_valid);
        } break;
        case "nativePtrStr"_sh: {
            std::string str = PointerToString(this);
            return jsi::String::createFromUtf8(runtime, str);
        } break;
        case "close"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 0, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                         {
                close();
                return jsi::Value::undefined();
            });
        } break;
        case "base64"_sh: {
            return jsi::Function::createFromHostFunction(runtime, name, 0, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments,
                                                                               size_t count) -> jsi::Value
                                                         {
                std::string res;
                if(count < 1) {
                    res = base64();
                }
                else {
                    jsi::Object obj = arguments[0].asObject(runtime);
                    jsi::Value fmtVal = obj.getProperty(runtime, "format");
                    if(fmtVal.isString()) {
                        jsi::String fmt = fmtVal.asString(runtime);
                        res = base64(fmt.utf8(runtime));
                    }
                    else {
                        res = "";
                    }
                }
                return jsi::String::createFromUtf8(runtime, std::move(res));
            });
        } break;
        case "md5"_sh: {
            return jsi::Function::createFromHostFunction
            (runtime, name, 0, [&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments, size_t count) -> jsi::Value
             {
                return jsi::String::createFromUtf8(runtime, md5());
            });
            
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

std::string PointerToString(void* cb)
{
    std::ostringstream oss;
    oss << reinterpret_cast<uintptr_t>(cb);
    return oss.str();
}

void* StringToPointer(std::string& str)
{
    std::istringstream iss(str);
    uintptr_t ptr;
    return (iss >> ptr) ? reinterpret_cast<void*>(ptr) : nullptr;
}

}
