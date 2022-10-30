CC = gcc
CFLAGS = -Wall -pedantic -pthread -v -lrt # Show all reasonable warnings
LDFLAGS =

all: simulator manager firealarm

simulator: Simulator.c
    gcc -o simulator Simulator.c $(CFLAGS)

manager: Manager.c
    gcc -o manager Manager.c $(CFLAGS)

firealarm: FireAlarm.c
    gcc -o firealarm FireAlarm.c $(CFLAGS)

clean:
    rm -f simulator manager firealarm *.o

.PHONY: all clean