#!/usr/bin/env cmake -P

if (NOT RC_VERSION)
  message(FATAL_ERROR "Specify -DRC_VERSION=<numeric-version>")
endif()

set(RELEASE_PLATFORMS darwin|win32|win64|trusty|xenial|joule|ostro|alloy|android-x32|android-x64)
if (NOT CMAKE_SYSTEM_NAME MATCHES "^(${RELEASE_PLATFORMS})$")
  message(FATAL_ERROR "Specify -DCMAKE_SYSTEM_NAME=<${RELEASE_PLATFORMS}>")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "^(android-x32|android-x64)$")
  if (NOT CMAKE_ANDROID_NDK)
    message(FATAL_ERROR "Specify -DCMAKE_ANDROID_NDK=<ndk-root>")
  endif()
  if (NOT MKLROOT)
    message(FATAL_ERROR "Specify -DMKLROOT=<mkl-root>")
  endif()
  set(RELEASE_EXTRA ${RELEASE_EXTRA}
    -DCMAKE_TOOLCHAIN_FILE=../cmake/Android.toolchain.cmake
    -DCMAKE_SYSTEM_VERSION=21
    -DCMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK}
    -DMKLROOT=${MKLROOT}
  )
elseif (MKLROOT)
  set(RELEASE_EXTRA ${RELEASE_EXTRA}
    -DMKLROOT=${MKLROOT}
  )
endif()

if (CMAKE_SYSTEM_NAME MATCHES "^(android-x64)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_ANDROID_ARCH_ABI=x86_64)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(android-x32)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_ANDROID_ARCH_ABI=x86)
else()
endif()

if (CMAKE_SYSTEM_NAME MATCHES "^(trusty)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_C_COMPILER=clang-3.6)
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_CXX_COMPILER=clang++-3.6)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "^(joule|alloy)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA}
      "-DCMAKE_C_FLAGS=-march=silvermont -mfpmath=sse"
      "-DCMAKE_CXX_FLAGS=-march=silvermont -mfpmath=sse")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "^(darwin|joule|alloy|trusty|xenial|ostro)$")
  set(RELEASE_GENERATOR "Ninja")
elseif(CMAKE_SYSTEM_NAME MATCHES "^(android-x32|android-x64)$")
  set(RELEASE_GENERATOR "Unix Makefiles")
elseif(CMAKE_SYSTEM_NAME MATCHES "^(win32)$")
  set(RELEASE_GENERATOR "Visual Studio 14 2015")
elseif(CMAKE_SYSTEM_NAME MATCHES "^(win64)$")
  set(RELEASE_GENERATOR "Visual Studio 14 2015 Win64")
else()
  message(FATAL_ERROR "GENERATOR?")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "^(joule|alloy|trusty|xenial|ostro|android-x32|android-x64)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_INSTALL_RPATH='$ORIGIN/../lib')
elseif(CMAKE_SYSTEM_NAME MATCHES "^(darwin)$")
  set(RELEASE_EXTRA ${RELEASE_EXTRA} -DCMAKE_INSTALL_RPATH=@executable_path/../lib)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(win32|win64)$")
else()
  message(FATAL_ERROR "RPATH?")
endif()

set(RELEASE_BUILD_DIR build-${RC_VERSION}-${CMAKE_SYSTEM_NAME})
message(STATUS "Directory ${RELEASE_BUILD_DIR}")
file(REMOVE_RECURSE "${RELEASE_BUILD_DIR}")
file(MAKE_DIRECTORY "${RELEASE_BUILD_DIR}")

execute_process(
  COMMAND "${CMAKE_COMMAND}"
    ..
    -G "${RELEASE_GENERATOR}"
    -DRC_VERSION=${RC_VERSION}
    -DIPPROOT=False
    -DCMAKE_BUILD_TYPE=Release
    -DCPACK_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DENABLE_SOURCES=True
    -DENABLE_STEREO=False
    -DENABLE_VISGL=False
    ${RELEASE_EXTRA}
  WORKING_DIRECTORY "${RELEASE_BUILD_DIR}"
  RESULT_VARIABLE RELEASE_CMAKE_RESULT
)
if (NOT RELEASE_CMAKE_RESULT EQUAL 0)
   message(FATAL_ERROR "error running cmake")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build . --config Release
  WORKING_DIRECTORY "${RELEASE_BUILD_DIR}"
  RESULT_VARIABLE RELEASE_BUILD_RESULT
)
if (NOT RELEASE_BUILD_RESULT EQUAL 0)
   message(FATAL_ERROR "error running cmake --build")
endif()

execute_process(
  COMMAND cpack --verbose -B "${CMAKE_CURRENT_SOURCE_DIR}"
  WORKING_DIRECTORY "${RELEASE_BUILD_DIR}"
  RESULT_VARIABLE RELEASE_CPACK_RESULT
)
if (NOT RELEASE_CPACK_RESULT EQUAL 0)
   message(FATAL_ERROR "error running cpack")
endif()
