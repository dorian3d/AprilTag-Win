cmake_minimum_required(VERSION 3.2.2)

set(CMAKE_SYSTEM_NAME Linux)

set(ANDROID_NDK "${ANDROID_NDK}" CACHE PATH "Android NDK Path")

set(ANDROID_PLATFORM "android-21" CACHE STRING "Android Platform: 21, 20, ...")
set(ANDROID_ARCH "x86" CACHE STRING "Android Architecture: x86, x86_64, arm64, ...")
if(ANDROID_ARCH STREQUAL "x86")
  set(CMAKE_SYSTEM_PROCESSOR i686)
else()
  set(CMAKE_SYSTEM_PROCESSOR ${ANDROID_ARCH})
endif()

set(ANDROID_TOOLCHAIN_MACHINE_NAME "${CMAKE_SYSTEM_PROCESSOR}-linux-android" CACHE INTERNAL "Toolchain target")

if (EXISTS "${CMAKE_BINARY_DIR}/../../toolchain/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
  set(ANDROID_TOOLCHAIN_ROOT "${CMAKE_BINARY_DIR}/../../toolchain" CACHE PATH "Toolchain root") # Compiler detection is run in ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp
else()
  set(ANDROID_TOOLCHAIN_ROOT "${CMAKE_BINARY_DIR}/toolchain" CACHE PATH "Toolchain root")
  if (NOT EXISTS "${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
    file(REMOVE_RECURSE "${ANDROID_TOOLCHAIN_ROOT}")
    execute_process(COMMAND "${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh"
                            --platform=${ANDROID_PLATFORM} --arch=${ANDROID_ARCH} --llvm-version=3.6 --stl=libcxx
                            --install-dir="${ANDROID_TOOLCHAIN_ROOT}")
    if (NOT EXISTS "${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
      message(ERROR "failed to build the toolchain: ${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
    endif()
  endif()
endif()

set(CMAKE_FIND_ROOT_PATH "${ANDROID_STANDALONE_TOOLCHAIN}/sysroot" "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_PREFIX}/share")

set(CMAKE_C_COMPILER   "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-clang"   CACHE PATH "C")
set(CMAKE_CXX_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-clang++" CACHE PATH "C++")

set(ANDROID True)
add_definitions(-DANDROID)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
