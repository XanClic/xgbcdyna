OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))
DYNAREC_PC = $(patsubst %.c,%.pc,$(wildcard dynarec/*.c))

LD = gcc
CC = gcc
MLIBS = `sdl-config --libs` -lrt $(LIBS)
MLDFLAGS = -m32 -L/usr/lib32 $(LDFLAGS) $(PROF)
MCFLAGS = -x c -std=c11 -O3 -Wall -Wextra -pedantic -g2 -m32 -march=native -mtune=native `sdl-config --cflags` $(CFLAGS) $(PROF)
PCFLAGS = -x c -std=c11 -O3 -Wall -Wextra -pedantic -g2 -m64
RM = rm -rf

.PHONY: all clean

.SUFFIXES:

all: xgbcdyna

xgbcdyna: $(OBJECTS)
	$(LD) $(MLDFLAGS) $^ -o $@ $(MLIBS)

%.o: %.pc xgbc.h
	$(CC) $(MCFLAGS) -c $< -o $@

execute.o: execute.pc $(DYNAREC_PC) xgbc.h
	$(CC) $(MCFLAGS) -c $< -o $@

%.pc: %.c precompiler.rb
	./precompiler.rb 32 $< $@

clean:
	$(RM) $(OBJECTS) $(patsubst %.c,%.pc,$(wildcard *.c)) $(DYNAREC_PC) xgbcdyna
