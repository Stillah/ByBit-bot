cmake_minimum_required(VERSION 3.28)
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING
"Vcpkg toolchain file")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASIO_STANDALONE -pthread")
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")
project(COMPLICATED)

set(CMAKE_CXX_STANDARD 23)

find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED) #Needed for part of boost/asio
find_package(Boost REQUIRED COMPONENTS asio beast json)

add_executable(COMPLICATED
        src/BotBase.cpp
        src/BotBase.h
        src/utility_functions.cpp
        src/utility_functions.h
        src/Strategy.h
        src/ByBit.cpp
        src/ByBit.h
)

target_sources(COMPLICATED PRIVATE src/main.cpp)

target_link_libraries(COMPLICATED PRIVATE
        ZLIB::ZLIB
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        ${Boost_LIBRARIES}
)

set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)


