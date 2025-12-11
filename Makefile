CC=gcc -O2 

.phony: all

FT_PKGS = harfbuzz cairo-ft freetype2

FT_CFLAGS = `pkg-config --cflags $(FT_PKGS)`
FT_LDFLAGS = `pkg-config --libs $(FT_PKGS)` -lm

all:zscore zscore2 zscore3

zscore: zscore.c ballon.c
	$(CC) -o zscore zscore.c ballon.c

zscore2: zscore2.c ballon.c
	$(CC) -o zscore2 zscore2.c ballon.c

zscore3: zscore3.c
	$(CC) zscore3.c $(FT_CFLAGS) $(FT_LDFLAGS) -o zscore3 

install:zscore zscore2 zscore3 zscore.bash
	cp -vf zscore zscore2 zscore3 zscore.bash ~/bin

