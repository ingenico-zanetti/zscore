CC=gcc -O2  -Wall

.phony: all

FT_PKGS = harfbuzz cairo-ft freetype2

FT_CFLAGS = `pkg-config --cflags $(FT_PKGS)`
FT_LDFLAGS = `pkg-config --libs $(FT_PKGS)` -lm

all:zscore zscore2 zscore3 crc xor beagle calculator

zscore: zscore.c ballon.c
	$(CC) -o zscore zscore.c ballon.c

zscore2: zscore2.c ballon.c
	$(CC) -o zscore2 zscore2.c ballon.c

zscore3: zscore3.c v200w_60x60_pixels.c
	$(CC) zscore3.c v200w_60x60_pixels.c $(FT_CFLAGS) $(FT_LDFLAGS) -o zscore3 

calculator: calculator.c
	$(CC) -o calculator calculator.c

beagle: beagle.c
	$(CC) -o beagle beagle.c

xor: xor.c
	$(CC) -o xor xor.c

crc: crc.c
	$(CC) -o crc crc.c

install:zscore zscore2 zscore3 zscore.bash
	cp -vf zscore zscore2 zscore3 zscore.bash ~/bin

