CC=g++
CFLAGS=-c -Wall -std=c++11 -Ofast
#lcrypt flag needs to be close to end (after objects) in command
LDFLAGS=-lcrypt
SOURCES=main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=decrypt

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
