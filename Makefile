OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

LD = gcc
CC = gcc
MLIBS = `sdl-config --libs` -lrt $(LIBS)
MLDFLAGS = -m32 -L/usr/lib32 $(LDFLAGS) $(PROF)
MCFLAGS = -std=c11 -O3 -Wall -Wextra -pedantic -g2 -m32 -march=native -mtune=native `sdl-config --cflags` $(CFLAGS) $(PROF)
RM = rm -rf

.PHONY: all clean

all: xgbcdyna

xgbcdyna: $(OBJECTS)
	$(LD) $(MLDFLAGS) $^ -o $@ $(MLIBS)

%.o: %.c xgbc.h
	$(CC) $(MCFLAGS) -c $< -o $@

execute.o: execute.c $(wildcard dynarec/*.c) xgbc.h
	$(CC) $(MCFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) xgbcdyna
