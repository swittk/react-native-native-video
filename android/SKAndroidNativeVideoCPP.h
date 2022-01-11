//
// Created by Switt Kongdachalert on 5/1/2022 AD.
//

#import "react-native-native-video.h"
#import <jni.h>
#ifndef ANDROID_SKANDROIDNATIVEVIDEOCPP_H
#define ANDROID_SKANDROIDNATIVEVIDEOCPP_H

namespace SKRNNativeVideo {
    class SKAndroidNativeFrameWrapper : public SKNativeFrameWrapper {
    public:
        JavaVM *jvm;
        JNIEnv *getJNIEnv() {
            JNIEnv *env;
            jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            return env;
//            jint result = jvm->AttachCurrentThread(&env, NULL);
//            assert(result == JNI_OK);
//            return env;
        }
        // Calls must be balanced between getJNIEnv and clearJNIEnv or you'll get memory shit like you've never seen
        void clearJNIEnv() {
//            jvm->DetachCurrentThread();
        }
        jobject bitmap;
        /** The bitmap is retained by this wrapper */
        SKAndroidNativeFrameWrapper(facebook::jsi::Runtime &_runtime, JavaVM *_vm, jobject _bitmap);
        ~SKAndroidNativeFrameWrapper();
        // Make sure to implement these methods!
        /** This is potentially for casting the correct type  (should return "iOS" for iOS and "Android" for Android)*/
        virtual std::string platform() { return "Android"; };
        // This should free/close native resources
        virtual void close();
        // Supposed to return ArrayBuffer
        virtual facebook::jsi::Value arrayBufferValue() { return facebook::jsi::Value::undefined(); }
        virtual SKRNSize size();
        virtual std::string base64(std::string format);
    };

    class SKAndroidNativeVideoWrapper : public SKNativeVideoWrapper {
    private:
//        jclass NativeVideoWrapperJavaSideClass;
        void initListHelper(JNIEnv *env);
        std::vector<jobject> javaList2vector_jobjects(JNIEnv *env, jobject javaList);
    public:
        JavaVM *jvm;
        jobject javaVideoWrapper;

        JNIEnv *getJNIEnv() {
            JNIEnv *env;
//            jint result = jvm->AttachCurrentThread(&env, NULL);
//            assert(result == JNI_OK);
//            return env;
            jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            return env;
        }
        // Calls must be balanced between getJNIEnv and clearJNIEnv or you'll get memory shit like you've never seen
        void clearJNIEnv() {
//            jvm->DetachCurrentThread();
        }

        SKAndroidNativeVideoWrapper(facebook::jsi::Runtime &runtime, std::string sourceUri, JavaVM* jvm);
        ~SKAndroidNativeVideoWrapper();

        virtual void close();

        virtual std::shared_ptr<SKNativeFrameWrapper>
        getFrameAtIndex(int index);

        virtual std::vector<std::shared_ptr<SKNativeFrameWrapper>> getFramesAtIndex(int index,
                                                                                    int numFrames);

        virtual std::shared_ptr<SKNativeFrameWrapper>
        getFrameAtTime(double time);

        virtual int numFrames();

        virtual double frameRate();

        virtual SKRNSize size();

        virtual double duration();
    };
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_reactnativenativevideo_SKRNNativeFrameViewManager_getBitmapFromStringAddressOfFrameWrapper(
                                                                                                    JNIEnv *env, jclass clazz, jstring address);
extern "C"
jint JNI_OnLoad(JavaVM *vm, void *reserved);

#endif //ANDROID_SKANDROIDNATIVEVIDEOCPP_H
