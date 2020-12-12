#
# Sample Makefile for CSE 130-02, Fall 2020
#

# The name of your program goes here.
TARGET=rpcserver

# A list of all your source files goes here.
# $(TARGET).cpp means to take the TARGET from above, and stick .cpp on the end.
#SOURCES=$(TARGET).cpp

# If you had multiple source files, you could do:
SOURCES=rpcserver.cpp hashTable.cpp functions.cpp

# These are the flags used to compile and build your code.
# This line gives you flags with debugging symbols, which you might find useful.
CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -Og

# These are the flags we're going to use when grading your assignment.
# CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -O2

#################################################################################
#
#   You shouldn't need to modify much below this line,
#   but you're welcome to do so if you want.
#
#################################################################################
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(SOURCES:.cpp=.d)

CXX=clang++

all: $(TARGET)

clean:
	-rm $(DEPS) $(OBJECTS)

spotless: clean
	-rm $(TARGET)

format:
	clang-format -i $(SOURCES) $(INCLUDES)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -pthread -o $@ $(OBJECTS)

-include $(DEPS)

.PHONY: all clean format spotless
