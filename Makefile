OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

LD = gcc
CC = gcc
LIBS = `sdl-config --libs` -lrt
LDFLAGS = -m32 -L/usr/lib32
CFLAGS = -std=c1x -O3 -Wall -Wextra -pedantic -g2 -m32 `sdl-config --cflags`
RM = rm -rf

.PHONY: all clean

all: xgbcdyna

xgbcdyna: $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) xgbcdyna
