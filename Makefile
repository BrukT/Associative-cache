CXX=g++
CPPFLAGS=-std=c++11

INC = -I$(SRC) -I$(SRC)/replacement/source

BIN = bin
BUILD = build
SRC = src

BUILD_CAC = $(BUILD)/cache
BUILD_REP = $(BUILD)/replacement/source
BUILD_SIM = $(BUILD)/sim
BUILD_SYS = $(BUILD)/system
BUILD_TST = $(BUILD)/test

SRC_CAC = $(SRC)/cache
SRC_REP = $(SRC)/replacement/source
SRC_SIM = $(SRC)/sim
SRC_SYS = $(SRC)/system
SRC_TST = $(SRC)/test

CAC_OBJ :=  $(patsubst $(SRC_CAC)/%.cpp,$(BUILD_CAC)/%.o,$(wildcard $(SRC_CAC)/*.cpp))
REP_OBJ :=  $(BUILD_REP)/ReplacementHandler.o  # $(patsubst $(SRC_REP)/%.cpp,$(BUILD_REP)/%.o,$(wildcard $(SRC_REP)/*.cpp))
SIM_OBJ :=  $(patsubst $(SRC_SIM)/%.cpp,$(BUILD_SIM)/%.o,$(wildcard $(SRC_SIM)/*.cpp))
SYS_OBJ :=  $(patsubst $(SRC_SYS)/%.cpp,$(BUILD_SYS)/%.o,$(wildcard $(SRC_SYS)/*.cpp))
TST_OBJ :=  $(patsubst $(SRC_TST)/%.cpp,$(BUILD_TST)/%.o,$(wildcard $(SRC_TST)/*.cpp))

.PHONY: directories

all: directories gitignores $(BIN)/sim $(BIN)/unit_test

directories:
	@mkdir -p $(BIN) $(BUILD) $(BUILD_SIM) $(BUILD_SYS) $(BUILD_CAC) $(BUILD_REP) $(BUILD_TST)

gitignores:
	@printf "*\n.gitignore" > $(BIN)/.gitignore
	@printf "*.o\n.gitignore" > $(BUILD)/.gitignore
	@printf "bin/\nbuild/\n.gitignore" > .gitignore

clean:
	rm -rf $(BIN) $(BUILD)

$(BUILD)/%.o:
	$(CXX) -c $(CPPFLAGS) $(INC) -o $@ $(SRC)/$*.cpp

$(BIN)/sim: $(SYS_OBJ) $(REP_OBJ) $(CAC_OBJ) $(SIM_OBJ)
	$(CXX) $(CPPFLAGS) -o $(BIN)/sim $^

$(BIN)/unit_test: $(SYS_OBJ) $(REP_OBJ) $(CAC_OBJ) $(TST_OBJ)
	$(CXX) $(CPPFLAGS) -o $(BIN)/unit_test $^