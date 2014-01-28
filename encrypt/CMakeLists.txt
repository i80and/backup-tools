cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

option(BUILD_TESTS "Build unit tests" ON)

project(wuffcrypt)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_BUILD 0-git)

add_definitions(-DWUFFCRYPT_VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}")

set(SODIUM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/libsodium)
ExternalProject_Add(libsodium
                    PREFIX ${SODIUM_DIR}/
                    SOURCE_DIR ${SODIUM_DIR}/
                    CONFIGURE_COMMAND ${SODIUM_DIR}/configure --prefix=${SODIUM_DIR}/ --enable-static --disable-shared
                    BUILD_COMMAND make
                    BUILD_IN_SOURCE 1
)

file(GLOB WUFFCRYPT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SCRYPT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/scrypt/*.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/scrypt/)
include_directories(${SODIUM_DIR}/include/)
link_directories(${SODIUM_DIR}/lib/)

set(SRC ${WUFFCRYPT_SRC} ${SCRYPT_SRC})
add_executable(wuffcrypt ${SRC})
add_dependencies(wuffcrypt libsodium)
target_link_libraries(wuffcrypt sodium)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # CMake tells Clang on Windows to add a .lib suffix that libsodium doesn't kick out
    set(CMAKE_LINK_LIBRARY_SUFFIX "")

    # Haiku requires that we explicitly link with fstack-protector
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wshadow -Wno-unused-parameter -fstack-protector")

    # Mac OS X's libstdc++ installation is awful; force libc++
    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

set(CMAKE_BUILD_TYPE Release)

# Testing stuff
if(BUILD_TESTS)
    add_subdirectory(tests)
    enable_testing()
endif()
add_test(NAME SecureString COMMAND tests/securestring)
add_test(NAME PaddedBuffer COMMAND tests/paddedbuffer)
