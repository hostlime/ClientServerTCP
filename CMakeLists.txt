cmake_minimum_required(VERSION 3.12)
project(MyProject)

# Установить стандарт C++ на C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Определяем суфикс для версии Debug
set(CMAKE_DEBUG_POSTFIX _g)

# Определяем флаги для компиляции файлов C++
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -std=c++17")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")


include_directories(include)

# Добавить клиентскую часть
add_subdirectory(client)

# Добавить серверную часть
add_subdirectory(server)
