cmake_minimum_required(VERSION 3.4.1)
project(reactnativenativevideo)

set(CMAKE_VERBOSE_MAKEFILE ON)
if(RNNATIVEVIDEO_NEW_ARCH)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NATIVE_VIDEO_SOURCES
  ../cpp/react-native-native-video.cpp
  SKAndroidNativeVideoCPP.cpp
  SKAndroidNativeVideoCPPOnLoad.cpp
)

if(RNNATIVEVIDEO_NEW_ARCH)
  list(APPEND NATIVE_VIDEO_SOURCES NativeVideoBindings.cpp)
else()
  list(APPEND NATIVE_VIDEO_SOURCES NativeVideoLegacyBindings.cpp)
endif()

add_library(reactnativenativevideo SHARED ${NATIVE_VIDEO_SOURCES})

target_compile_definitions(
  reactnativenativevideo
  PRIVATE
  RNNATIVEVIDEO_NEW_ARCH=${RNNATIVEVIDEO_NEW_ARCH}
)

target_include_directories(
  reactnativenativevideo
  PRIVATE
  .
  ../cpp
  "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
  "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni/react/turbomodule"
)

find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(log-lib log)
find_library(dl-lib dl)
find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

# RN 0.76+ folds the legacy JNI libraries into `reactnative`; RN 0.73 exposes
# the equivalent symbols through `reactnativejni`.
if(TARGET ReactAndroid::reactnative)
  set(REACT_NATIVE_RUNTIME_TARGET ReactAndroid::reactnative)
elseif(TARGET ReactAndroid::reactnativejni)
  set(REACT_NATIVE_RUNTIME_TARGET ReactAndroid::reactnativejni)
else()
  message(FATAL_ERROR "ReactAndroid did not expose a supported runtime target")
endif()

target_link_libraries(
  reactnativenativevideo
  ReactAndroid::jsi
  ${REACT_NATIVE_RUNTIME_TARGET}
  fbjni::fbjni
  ${android-lib}
  ${jnigraphics-lib}
  ${log-lib}
  ${dl-lib}
)

target_link_options(
  reactnativenativevideo
  PRIVATE
  "-Wl,-u,SKRNNativeVideoAcquireNativeBufferLease"
  "-Wl,-u,SKRNNativeVideoNativeBufferLeaseGetPointer"
  "-Wl,-u,SKRNNativeVideoNativeBufferLeaseGetType"
  "-Wl,-u,SKRNNativeVideoReleaseNativeBufferLease"
)
