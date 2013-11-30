CC=g++
CFLAGS=-c -Wall -std=c++11 -Ofast -fopenmp
#lcrypt flag needs to be close to end (after objects) in command
LDFLAGS=-lcrypt -fopenmp
SOURCES=main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=decrypt

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
