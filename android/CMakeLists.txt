cmake_minimum_required(VERSION 3.4.1)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 14)
set (NODE_MODULES_DIR "../node_modules")
set (RN_SO_DIR ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni/first-party/react/jni)
set (FBJNI_HEADERS_DIR ${RN_SO_DIR}/../../fbjni/headers)

#if(${REACT_NATIVE_TARGET_VERSION} LESS 66)
set (
    INCLUDE_JSI_CPP
    "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp"
)
#  set (
#          INCLUDE_JSIDYNAMIC_CPP
#          "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/JSIDynamic.cpp"
#  )
#endif()

set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
set (PACKAGE_NAME "reactnativenativevideo")


add_library(${PACKAGE_NAME}
            SHARED
            ${INCLUDE_JSI_CPP}
            ${INCLUDE_JSIDYNAMIC_CPP}
            ../cpp/react-native-native-video.cpp
            cpp-adapter.cpp
            SKAndroidNativeVideoCPP.cpp
)

# Specifies a path to native header files.
include_directories(
            ../cpp
            .
            "${FBJNI_HEADERS_DIR}"
            ../node_modules/react-native/React
            ../node_modules/react-native/React/Base
            ../node_modules/react-native/ReactCommon
            ../node_modules/react-native/ReactCommon/jsi
            ../node_modules/react-native/ReactCommon/callinvoker
            ../node_modules/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon
            # ../node_modules/react-native/ReactAndroid/src/main/jni/react/jni/JavaScriptExecutorHolder.h
            # ../node_modules/react-native/ReactAndroid/src/main/jni/react/jni
            ../node_modules/react-native/ReactAndroid/src/main/jni/react
            ../node_modules/react-native/ReactCommon/turbomodule/core
            ../node_modules/react-native/ReactCommon/turbomodule
            ../node_modules/react-native/React
            ../node_modules/react-native/React/Base
            ../node_modules/react-native/ReactCommon/jsi
)
