CXX = g++
INC = -I./src/include
CXXFLAGS = -g -O2 $(INC) -Wall -Wunused-variable
LDFLAGS = -ltorrent-rasterbar -rdynamic

BUILD_DIR = build
SRC_DIR = src

SOURCES = main.cpp cmd.cpp util.cpp
OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

TARGET = $(BUILD_DIR)/mintd

all: $(TARGET)

clean:
	rm -rf $(BUILD_DIR)

fresh: clean all

$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all clean fresh
