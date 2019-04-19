CXX=g++
CPPFLAGS=-std=c++11

INC = -I$(SRC)

BIN = bin
BUILD = build
SRC = src

BUILD_CAC = $(BUILD)/cache
BUILD_SIM = $(BUILD)/sim
BUILD_SYS = $(BUILD)/system

SRC_CAC = $(SRC)/cache
SRC_SIM = $(SRC)/sim
SRC_SYS = $(SRC)/system

CAC_OBJ :=  $(patsubst $(SRC_CAC)/%.cpp,$(BUILD_CAC)/%.o,$(wildcard $(SRC_CAC)/*.cpp))
SIM_OBJ :=  $(patsubst $(SRC_SIM)/%.cpp,$(BUILD_SIM)/%.o,$(wildcard $(SRC_SIM)/*.cpp))
SYS_OBJ :=  $(patsubst $(SRC_SYS)/%.cpp,$(BUILD_SYS)/%.o,$(wildcard $(SRC_SYS)/*.cpp))

.PHONY: directories

all: directories gitignores $(BIN)/sim

directories:
	@mkdir -p $(BIN) $(BUILD) $(BUILD_SIM) $(BUILD_SYS) $(BUILD_CAC)

gitignores:
	@printf "*\n.gitignore" > $(BIN)/.gitignore
	@printf "*.o\n.gitignore" > $(BUILD)/.gitignore
	@printf "bin/\nbuild/\n.gitignore" > .gitignore

clean:
	rm -rf $(BIN) $(BUILD)

$(BUILD)/%.o:
	$(CXX) -c $(CPPFLAGS) $(INC) -o $@ $(SRC)/$*.cpp

$(BIN)/sim: $(SYS_OBJ) $(CAC_OBJ) $(SIM_OBJ)
	$(CXX) $(CPPFLAGS) -o $(BIN)/sim $^
