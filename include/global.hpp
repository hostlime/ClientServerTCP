#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <memory>
#include <string>
#include <iostream>
#include <filesystem>
#include <array>


#ifdef _WIN32
 #define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


#include<TcpPackage.hpp>

#define  DEFAULT_PORT  12345
#define  DEFAULT_HOST  "localhost"
#endif