CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
LDFLAGS  ?= -pthread

HEADERS := $(wildcard core/*.h graph/*.h authentication/*.h)

.PHONY: all clean reset-db

all: server/server server/client

server/server: server/server.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

server/client: server/client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f server/server server/client

# Deletes the on-disk database so the next `--load` starts from scratch.
reset-db:
	rm -rf server/*.dat server/ratings
