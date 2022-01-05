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
        // Make sure to implement these methods!
        /** This is potentially for casting the correct type  (should return "iOS" for iOS and "Android" for Android)*/
        virtual std::string platform() { return "Android"; };
        // This should free/close native resources
        virtual void close();
        // Supposed to return ArrayBuffer
        virtual facebook::jsi::Value arrayBufferValue() { return facebook::jsi::Value::undefined(); }
        virtual SKRNSize size();
    };

    class SKAndroidNativeVideoWrapper : public SKNativeVideoWrapper {
    private:
//        jclass NativeVideoWrapperJavaSideClass;
        jmethodID NativeVideoWrapperJavaGetFrameAtIndexMethod = 0;
        jmethodID NativeVideoWrapperJavaGetFramesAtIndexMethod = 0;
        jmethodID NativeVideoWrapperJavaGetFrameAtTimeMethod = 0;
        jmethodID NativeVideoWrapperJavaGetNumFramesMethod = 0;
        jmethodID NativeVideoWrapperJavaGetFrameRateMethod = 0;
        jmethodID NativeVideoWrapperJavaGetDurationMethod = 0;
        jmethodID NativeVideoWrapperJavaGetWidthMethod = 0;
        jmethodID NativeVideoWrapperJavaGetHeightMethod = 0;

        jclass java_util_List;
        jmethodID java_util_List_;
        jmethodID java_util_List_size;
        jmethodID java_util_List_get;
        jmethodID java_util_List_add;
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


#endif //ANDROID_SKANDROIDNATIVEVIDEOCPP_H
