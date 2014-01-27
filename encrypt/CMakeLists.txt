cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

project(wuffcrypt)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_BUILD 0)

set(SODIUM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/libsodium)
ExternalProject_Add(libsodium
                    PREFIX ${SODIUM_DIR}/
                    SOURCE_DIR ${SODIUM_DIR}/
                    CONFIGURE_COMMAND ${SODIUM_DIR}/configure --prefix=${SODIUM_DIR}/ --enable-static --disable-shared
                    BUILD_COMMAND make
                    BUILD_IN_SOURCE 1
)

set(WUFFCRYPT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/wuffcrypt.cpp)
file(GLOB SCRYPT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/scrypt/*.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/scrypt/)
include_directories(${SODIUM_DIR}/include/)
link_directories(${SODIUM_DIR}/lib/)

set(SRC ${WUFFCRYPT_SRC} ${SCRYPT_SRC})
add_executable(wuffcrypt ${SRC})
add_dependencies(wuffcrypt libsodium)
target_link_libraries(wuffcrypt sodium)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wshadow -Wno-unused-parameter")
endif()

set(CMAKE_BUILD_TYPE Release)
