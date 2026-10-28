CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -O2 -IKeyPill

SRC = tests/test_runner.cpp KeyPill/aes128.cpp KeyPill/password_gen.cpp KeyPill/storage.cpp
TARGET = test_runner

all: test

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

web:
	./start_web.sh

serve: web

clean:
	rm -f $(TARGET)

.PHONY: all test web serve clean
