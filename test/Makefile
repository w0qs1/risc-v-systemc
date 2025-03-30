# Set SystemC paths
SYSTEMC_HOME ?= /home/ubuntu/systemc
CXX          = g++
CXXFLAGS     = -I$(SYSTEMC_HOME)/include -L$(SYSTEMC_HOME)/lib-linux64 -lsystemc -lm

# Define source directory (default to current directory if not specified)
SRC_DIR ?= .

# Find all .cpp files in the specified source directory
SRC  = $(wildcard $(SRC_DIR)/*.cpp)
OBJ  = $(SRC:.cpp=.o)
TARGET = $(SRC_DIR)/out  # Change this to any desired executable name

# Default target
all: $(TARGET)

# Compile and link
$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# Compile source files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Run the simulation
run: $(TARGET)
	./$(TARGET)

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)

# Usage example:
# make all SRC_DIR=path/to/source
