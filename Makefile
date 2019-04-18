CXX=g++
CPPFLAGS=-std=c++11

BIN = bin
BUILD = build
SRC = src
BUILD_SIM = $(BUILD)/sim
SRC_SIM = $(SRC)/sim

SIM_OBJ :=  $(patsubst $(SRC_SIM)/%.cpp,$(BUILD_SIM)/%.o,$(wildcard $(SRC_SIM)/*.cpp))

.PHONY: directories

all: directories gitignores $(BIN)/sim

directories:
	@mkdir -p $(BIN) $(BUILD) $(BUILD_SIM)

gitignores:
	@printf "*\n.gitignore" > $(BIN)/.gitignore
	@printf "*.o\n.gitignore" > $(BUILD)/.gitignore
	@printf "bin/\nbuild/\n.gitignore" > .gitignore

clean:
	rm -rf $(BIN) $(BUILD)

$(BUILD)/%.o:
	$(CXX) $(CPPFLAGS) -c -o $@ $(SRC)/$*.cpp

$(BIN)/sim: $(SIM_OBJ)
	$(CXX) $(CPPFLAGS) -o $(BIN)/sim $(SIM_OBJ)