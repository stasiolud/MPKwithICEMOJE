# Makefile for SIP system

# Ice configuration
ICE_LIBS = -lIce -lIceUtil
ICE_INCLUDE = -I/usr/include/ice
SLICE2CPP = slice2cpp

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g -pthread $(ICE_INCLUDE)

# Directories
BUILD_DIR = build
SRC_DIR = src

# Slice file
SLICE_FILE = SIP.ice
SLICE_GEN = SIP.cpp SIP.h

# Source files
SYSTEM_SRC = SystemMain.cpp MPKImpl.cpp
TRAM_SRC = TramImpl.cpp
PASSENGER_SRC = PassengerImpl.cpp

# Object files
SYSTEM_OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SYSTEM_SRC))
TRAM_OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TRAM_SRC))
PASSENGER_OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(PASSENGER_SRC))
SLICE_OBJ = $(BUILD_DIR)/SIP.o

# Executables
SYSTEM_EXE = system
TRAM_EXE = tram
PASSENGER_EXE = passenger

# Default target
all: $(BUILD_DIR) $(SLICE_GEN) $(SYSTEM_EXE) $(TRAM_EXE) $(PASSENGER_EXE)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate code from Slice file
$(SLICE_GEN): $(SLICE_FILE)
	$(SLICE2CPP) $(SLICE_FILE)

# Compile Slice generated code
$(SLICE_OBJ): SIP.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile system source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(SLICE_GEN)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link system executable
$(SYSTEM_EXE): $(SYSTEM_OBJ) $(SLICE_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(ICE_LIBS) -o $@

# Link tram executable
$(TRAM_EXE): $(TRAM_OBJ) $(SLICE_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(ICE_LIBS) -o $@

# Link passenger executable
$(PASSENGER_EXE): $(PASSENGER_OBJ) $(SLICE_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(ICE_LIBS) -o $@

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(SLICE_GEN) $(SYSTEM_EXE) $(TRAM_EXE) $(PASSENGER_EXE)

.PHONY: all clean
