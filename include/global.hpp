#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <memory>
#include <string>
#include <iostream>
#include <filesystem>
#include <array>

/*
#include<boost/serialization/serialization.hpp>
#include<boost/serialization/nvp.hpp>

#include<boost/serialization/string.hpp>
#include<boost/serialization/vector.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
*/
#ifdef _WIN32
 #define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


#include<CustomSerializ.hpp>
#include<TcpPackage.hpp>

#define  DEFAULT_PORT  12345

#endif