CC=g++
CFLAGS=-c -Wall
LDFLAGS=-lasound -lSDL2 -lstdc++
SOURCES=combine.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=combine

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
