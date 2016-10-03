VERSION=0.1.1-pre-`git rev-parse --short HEAD`

FLAGS=-Wall -Wextra -Wshadow -O2 -fstack-protector-all -DWUFFCRYPT_VERSION=\"$(VERSION)\"
CFLAGS=$(FLAGS) -std=c99 -fPIC `pkg-config --cflags libsodium`
CXXFLAGS=$(FLAGS) -std=c++11 -fpie -lsodium `pkg-config --cflags --libs libsodium`

SRC=src/arguments.cpp \
    src/main.cpp \
    src/util.cpp \
    src/wuffcrypt.cpp \

SRC_SCRYPT=src/thirdparty/scrypt/crypto_scrypt-ref.c \
           src/thirdparty/scrypt/sha256.c
OBJ_SCRYPT=$(SRC_SCRYPT:.c=.o)

SRC_TESTS=tests/test_paddedbuffer.cpp \
          tests/test_securestring.cpp
TESTS=$(SRC_TESTS:.cpp=)

.PHONY: clean test lint

wuffcrypt: $(SRC) $(OBJ_SCRYPT)
	$(CXX) $(CXXFLAGS) -o $@ -I src/thirdparty/scrypt $(SRC) $(OBJ_SCRYPT)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

tests/%: tests/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -I src/ $^ src/util.cpp

clean:
	rm -Rf wuffcrypt
	rm -f $(TESTS)
	find ./src -name "*.o" -exec rm {} \;

lint:
	cppcheck --enable=all --inconclusive $(SRC)

test: $(TESTS)
	for test in $(TESTS); do echo "Starting $$test" && ./$$test; done
