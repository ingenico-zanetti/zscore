    /*
     * Read a file containing some frame, with the count at start of each line
     * If the count is big enough, create a file with the content of the frame, excluding the starting F1 (supposed to be a sync word) and the ending 8F (invariant)
     * the file is named after the line number and the size
     *

    377 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 96 55 AA 95 55 AA 55 AA AA AA AA 66 59 65 56 9A 59 A6 96 8F 
    378 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 56 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 59 56 AA 65 55 AA 55 AA AA AA AA AA AA AA AA 69 9A 69 55 8F 
    384 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 65 55 AA 56 55 AA 55 66 59 69 56 66 59 65 56 A5 6A A9 99 8F 
    386 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 6A 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 59 59 AA 66 56 AA 55 AA AA AA AA 66 59 65 56 6A 69 A9 9A 8F 
    389 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 55 56 AA 95 55 AA 55 AA AA AA AA 66 59 65 56 59 5A 56 5A 8F 
    400 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 66 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 56 59 AA 59 56 AA 55 AA AA AA AA AA AA AA AA 6A AA 56 AA 8F 
    403 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 65 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 5A 55 AA 65 55 AA 55 AA AA AA AA 66 59 65 56 59 66 A5 5A 8F 
    408 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 66 56 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 95 56 AA 6A 55 AA 55 66 59 69 56 66 59 65 56 A5 5A A6 55 8F 
    414 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 66 56 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 55 59 AA 95 55 AA 55 66 59 69 56 66 59 65 56 6A 55 59 96 8F 
    425 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 66 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 5A 59 AA 5A 56 AA 55 AA AA AA AA AA AA AA AA 65 AA 95 5A 8F 
    444 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 5A 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 55 59 AA 5A 56 AA 55 AA AA AA AA 66 59 65 56 69 56 66 A6 8F 
    447 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 66 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 95 56 AA 69 55 AA 55 AA AA AA AA AA AA AA AA A9 A5 96 95 8F 
    468 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 66 56 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 56 59 AA 55 56 AA 55 66 59 69 56 66 59 65 56 66 55 69 65 8F 
    489 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 5A 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 66 59 AA 69 56 AA 55 AA AA AA AA 66 59 65 56 5A 56 A5 9A 8F 
    510 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 5A 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 56 59 AA 5A 56 AA 55 AA AA AA AA 66 59 65 56 6A 56 96 A6 8F 
    525 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 56 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 66 59 AA 65 56 AA 55 AA AA AA AA AA AA AA AA 55 95 59 9A 8F 
    527 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 66 55 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 59 56 AA 69 55 AA 55 66 59 69 56 66 59 65 56 99 5A 56 69 8F 
    566 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 66 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 55 56 AA 65 55 AA 55 AA AA AA AA AA AA AA AA 6A A5 9A 55 8F 
    579 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 66 56 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 66 59 AA 5A 56 AA 55 66 59 69 56 66 59 65 56 56 55 66 65 8F 
    622 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 65 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 69 55 AA 95 55 AA 55 AA AA AA AA 66 59 65 56 6A 66 A5 A5 8F 
    623 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 56 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 65 59 AA 65 56 AA 55 AA AA AA AA AA AA AA AA 56 95 A9 9A 8F 
    679 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 66 55 AA 65 55 AA 55 AA AA AA AA 66 59 65 56 6A 59 56 55 8F 
   1018 serialDecode: score (l=58), F1 A5 56 55 55 55 55 55 59 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 55 55 AA 55 55 AA 55 AA AA AA AA 66 59 65 56 5A 56 99 55 8F 
   1254 serialDecode: score (l=58), F1 A5 56 55 55 55 55 55 59 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 55 55 AA 55 55 AA 55 66 59 69 56 66 59 65 56 96 65 A5 AA 8F 
   1310 serialDecode: score (l=58), F1 A5 56 55 55 55 55 55 59 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 5A AA AA AA 5A 55 55 AA 55 55 AA 55 66 59 69 56 66 59 65 56 96 A6 66 AA 8F 
   1682 serialDecode: score (l=58), F1 A5 56 55 55 55 55 55 59 55 65 55 55 55 55 55 55 56 55 55 65 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 55 55 AA 55 55 AA 55 AA AA AA AA AA AA AA AA 9A A9 96 5A 8F 
   2036 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 56 AA AA AA 56 55 55 AA 55 55 AA 55 AA AA AA AA 66 59 65 56 5A 59 96 9A 8F 
   2504 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 59 AA AA AA 59 55 55 AA 55 55 AA 55 66 59 69 56 66 59 65 56 96 6A AA 65 8F 
   2619 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 65 55 55 55 55 55 55 55 55 55 55 AA 55 AA 5A AA AA AA 5A 55 55 AA 55 55 AA 55 66 59 69 56 66 59 65 56 96 A9 69 65 8F 
   3362 serialDecode: score (l=58), F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 55 55 55 56 55 55 65 55 55 55 55 55 55 55 55 55 AA 55 AA 55 AA AA AA 55 55 55 AA 55 55 AA 55 AA AA AA AA AA AA AA AA 9A A6 99 95 8F 
  36802 serialDecode: clock (l=16), F1 6A 55 55 55 55 55 65 59 56 65 5A 55 95 6A 8F 

  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[]){
	int threshold = 2000;
	if(argc > 1){
		sscanf(argv[1], "%d", &threshold);
	}
	char ligne[1024];
	int lineNumber = 0;
	while(fgets(ligne, sizeof(ligne) - 1, stdin)){
		lineNumber++;
		int count = 0;
		sscanf(ligne, "%d", &count);
		fprintf(stderr, "Got line: [%s] with count=%d" "\n", ligne, count);
		if(count > threshold){
			char *coma = strchr(ligne, ',');
			if(coma){
				fprintf(stderr, "coma detected" "\n");
				char filename[32];
				snprintf(filename, sizeof(filename) - 1, "%06d.%04d.crc", count, lineNumber);
				FILE *f = fopen(filename, "wb+");
				if(f){
					// const char *s = coma + 5; // Skip F1
					const char *s = coma + 2; // include F1
					for(;;){
						unsigned int hexa;
						if((0 == sscanf(s, "%2X", &hexa)) || (0xF1 == hexa)){
							break;
						}
						unsigned char octet = (unsigned char)hexa;
						fwrite(&octet, 1, 1, f);
						s += 3;
					}
					fclose(f);
				}
			}
		}
	}
	return(0);
}

