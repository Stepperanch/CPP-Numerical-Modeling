# Makefile for C++ projects
# Usage:
#   make              # Build the program
#   make clean        # Remove build artifacts
#   make run          # Build and run
#   make debug        # Build with debug symbols
#   make release      # Build optimized version

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra
DEBUG_FLAGS = -O0 -g
RELEASE_FLAGS = -O3 -DNDEBUG

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = obj

# Source files (add your .cpp files here)
SOURCES = main.cpp
# For multi-file projects, uncomment and modify:
# SOURCES = main.cpp src/vector3d.cpp src/particle.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
TARGET = $(BIN_DIR)/program

# Default target
all: $(TARGET)

# Build the program
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Release build
release: CXXFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS)
	rm -rf $(BIN_DIR)/*
	@echo "Clean complete"

# Remove all generated files
distclean: clean
	rm -rf $(OBJ_DIR)

# Phony targets
.PHONY: all debug release run clean distclean

# Example multi-file project structure:
# Uncomment and modify when you have multiple files:
#
# SOURCES = main.cpp \
#           src/vector3d.cpp \
#           src/matrix.cpp \
#           src/particle.cpp \
#           src/integrator.cpp
#
# INCLUDES = -I$(INCLUDE_DIR)
#
# %.o: %.cpp
#     $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
