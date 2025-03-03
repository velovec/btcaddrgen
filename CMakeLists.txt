cmake_minimum_required(VERSION 2.7.1)

project(btcaddrgen)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Modules)

find_package(Boost REQUIRED program_options)
find_package(OpenSSL REQUIRED)
find_library(ECDSAXX ecdsa++)
# find_library(BLOOM bloom)

include_directories(
    "./"
    ${OPENSSL_INCLUDE_DIR}
)

file(GLOB src "./src/*.cc")

add_executable(btcaddrgen ${src})

target_link_libraries(btcaddrgen ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ECDSAXX})
