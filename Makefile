CC=g++
CFLAGS=-c -Wall
LDFLAGS=-lasound -lstdc++
SOURCES=holepunchrtp2.cpp holepunch.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=soundweave

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
