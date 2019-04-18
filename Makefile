CXX=g++
CPPFLAGS=-std=c++11

BIN = bin
BUILD = build
SRC = src
SRC_SIM = $(SRC)/sim

SIM_OBJ := $(patsubst %.cpp,%.o,$(wildcard $(SRC_SIM)/*.cpp))

sim: $(SIM_OBJ)
	$(CXX) $(CPPFLAGS) -o sim $(SIM_OBJ)

clean:
	rm -f $(SIM_OBJ)