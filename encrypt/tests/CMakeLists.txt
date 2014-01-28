include_directories(${wuffcrypt_SOURCE_DIR}/src/)
add_executable(securestring test_securestring.cpp ${wuffcrypt_SOURCE_DIR}/src/util.cpp)
add_executable(paddedbuffer test_paddedbuffer.cpp ${wuffcrypt_SOURCE_DIR}/src/util.cpp)

target_link_libraries(securestring sodium)
