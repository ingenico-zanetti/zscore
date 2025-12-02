.phony: all

all:zscore zscore2

zscore: zscore.c ballon.c
	$(CC) -o zscore zscore.c ballon.c

zscore2: zscore2.c ballon.c
	$(CC) -o zscore2 zscore2.c ballon.c

install:zscore zscore2 zscore.bash
	cp -vf zscore zscore2 zscore.bash ~/bin

