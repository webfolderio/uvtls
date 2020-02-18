cmake_minimum_required(VERSION 3.5)

if(DEFINED ENV{VCPKG_INSTALLATION_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(uvtls VERSION 0.1 DESCRIPTION "TLS for libuv")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

####

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(TLS_BACKEND "TLS/SSL library to use for the backend")

####

include(FindLibuv)

list(APPEND LIBRARIES ${LIBUV_LIBRARIES})
list(APPEND INCLUDE_DIRS ${LIBUV_INCLUDE_DIRS})

####

if(${TLS_BACKEND} STREQUAL "OpenSSL" OR NOT TLS_BACKEND)
  find_package(OpenSSL)

  if (OPENSSL_FOUND)
    set(TLS_BACKEND "OpenSSL")
  endif()

  list(APPEND LIBRARIES ${OPENSSL_LIBRARIES})
  list(APPEND INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
else()
  message(FATAL_ERROR "Unsupport TLS backend: ${TLS_BACKEND}")
endif()

####

list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

####

add_subdirectory(src)

if (BUILD_TESTS)
  add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
