zscore: zscore.c ballon.c
	$(CC) -o zscore zscore.c ballon.c

install:zscore
	cp -vf zscore ~/bin

