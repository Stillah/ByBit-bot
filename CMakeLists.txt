#ЛУЧШЕ НИЧЕГО НЕ ТРОГАТЬ

cmake_minimum_required(VERSION 3.28)
#set(cpr_DIR "C:/Users/shami/CLionProjects/COMPLICATED/vcpkg/packages/cpr_x64-linux/share/cpr/cprConfig.cmake")
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING
"Vcpkg toolchain file")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASIO_STANDALONE -pthread")
SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
project(COMPLICATED)

set(CMAKE_CXX_STANDARD 23)

#find_path(UWEBSOCKETS_INCLUDE_DIRS "uwebsockets/App.h")
#find_path(USOCKET_LIB_A "lib/libuSockets.a")

find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED) #Needed for part of boost/asio
find_package(Boost REQUIRED COMPONENTS asio beast json)

#find_package(cpr REQUIRED)
#find_package(httplib REQUIRED)
#include_directories("${UWEBSOCKETS_INCLUDE_DIRS}/uwebsockets")

add_executable(COMPLICATED
        src/BotBase.cpp
        src/BotBase.h
        src/Encryption.hpp
        src/Strategy.hpp
        src/ByBit.cpp
        src/ByBit.h
)

target_sources(COMPLICATED PRIVATE src/main.cpp)

target_link_libraries(COMPLICATED PRIVATE
        ZLIB::ZLIB OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        ${Boost_LIBRARIES}
        #"${USOCKET_LIB_A}/lib/libuSockets.a"
        #cpr::cpr
        #httplib::httplib)
)

set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)


