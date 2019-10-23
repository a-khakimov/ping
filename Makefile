CC=gcc
SOURCES=main.c _ping.c
CFLAGS=-c -Wall
LDFLAGS=
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ping

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@


.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf ping *.o

install:


