#SET(CMAKE_TOOLCHAIN_FILE "C:/Program Files/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(MINGW)
  include(CheckLibraryExists)
  check_library_exists(ws2_32 AcceptEx "" HAVE_ACCEPT_EX)
  check_library_exists(ws2_32 GetAcceptExSockaddrs "" HAVE_GET_ACCEPT_EX_SOCK_ADDRS)
  if(HAVE_ACCEPT_EX AND HAVE_GET_ACCEPT_EX_SOCK_ADDRS)
    add_definitions(-D_WIN32_WINNT=0x0600)
    target_link_libraries(Server ws2_32)
  endif()
endif()
cmake_minimum_required(VERSION 3.16)
project(Server)

find_library(WS2_32_LIB ws2_32)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(Server ${SOURCE_FILES})

target_include_directories(Server PRIVATE include)

# Настроить библиотеку Asio
set(ASIO_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/asio-1.24.0/include")
find_library(ASIO_LIB asio ${ASIO_INCLUDE_DIR})
#include_directories(${ASIO_INCLUDE_DIR})

if(MINGW)
 target_link_libraries(Server ws2_32)
endif()

