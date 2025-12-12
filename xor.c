#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]){
	if(argc < 4){
		fprintf(stderr, "Usage: %s <file1> <file2> <file1^file2>" "\n", argv[0]);
		fprintf(stderr, "Example: %s file1.bin file1.bin file1_xor_file2.bin" "\n", argv[0]);
	}else{
		FILE *f1 = fopen(argv[1], "rb");
		FILE *f2 = fopen(argv[2], "rb");
		FILE *xor = fopen(argv[3],"wb+");

		if((NULL != f1) && (NULL !=f2) && (NULL != xor)){
			for(;;){
				unsigned char octet1;
				unsigned char octet2;
				int count = 0;
				count += fread(&octet1, 1, 1, f1);
				count += fread(&octet2, 1, 1, f2);
				if(2 == count){
					octet1 ^= octet2;
					fwrite(&octet1, 1, 1, xor);
				}else{
					break;
				}
			}
		}

		if(NULL != f1){
			fclose(f1);
		}
		if(NULL != f2){
			fclose(f2);
		}
		if(NULL != xor){
			fclose(xor);
		}
	}
	return(0);
}

