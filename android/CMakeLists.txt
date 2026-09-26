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
  if(REACT_NATIVE_MINOR_VERSION LESS 66)
    list(APPEND NATIVE_VIDEO_SOURCES
      "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp"
    )
  endif()
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
)

find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(log-lib log)

if(RNNATIVEVIDEO_NEW_ARCH)
  find_package(ReactAndroid REQUIRED CONFIG)
  find_package(fbjni REQUIRED CONFIG)
  target_link_libraries(
    reactnativenativevideo
    ReactAndroid::jsi
    ReactAndroid::reactnative
    fbjni::fbjni
    ${android-lib}
    ${jnigraphics-lib}
    ${log-lib}
  )
else()
  set(REACT_NATIVE_DIR "${NODE_MODULES_DIR}/react-native")
  target_include_directories(
    reactnativenativevideo
    PRIVATE
    "${REACT_NATIVE_DIR}/React"
    "${REACT_NATIVE_DIR}/React/Base"
    "${REACT_NATIVE_DIR}/ReactCommon"
    "${REACT_NATIVE_DIR}/ReactCommon/jsi"
    "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  )
  target_link_libraries(
    reactnativenativevideo
    ${android-lib}
    ${jnigraphics-lib}
    ${log-lib}
  )
endif()
