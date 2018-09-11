CONFIGSRC = ./Config/config.cpp
RANDOMSRC = ./Randomizer/randomizer.cpp
CFLAGS = -std=c++17 -O3 -g -I .

default: tinyhost

tinyhost: tinyhost.cpp
	g++ tinyhost.cpp $(CONFIGSRC) $(RANDOMSRC) -o tinyhost $(CFLAGS)