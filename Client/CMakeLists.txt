cmake_minimum_required(VERSION 3.16)
project(Client)

find_library(WS2_32_LIB ws2_32)

# Установить стандарт C++ на C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(Client ${SOURCE_FILES})

target_include_directories(Client PRIVATE include)

# Настроить библиотеку Asio
set(ASIO_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/asio-1.24.0/include")
find_library(ASIO_LIB asio ${ASIO_INCLUDE_DIR})


if(MINGW)
 target_link_libraries(Client ws2_32)
endif()