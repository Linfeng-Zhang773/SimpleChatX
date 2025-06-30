# Makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Iincludes -Wall -g

# Target executable
TARGET = ChatServer

# Source and object files
SRCDIR = srcs
OBJDIR = objs
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SOURCES))

# Default target
all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile each .cpp file into .o
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean generated files
clean:
	rm -rf $(OBJDIR) $(TARGET)
