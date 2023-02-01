CC=g++
CFLAGS=-c -Wall
LDFLAGS=-lasound -lstdc++
SOURCES=holepunchrtp2.cpp holepunch.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=program

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
