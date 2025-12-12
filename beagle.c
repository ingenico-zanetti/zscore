/*
 * Generate a python script that try to reverse engineer the CRC
 * using CRC beagle 
 *


 *
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]){
	printf("from crcbeagle import crcbeagle" "\n");

	printf("crcb = crcbeagle.CRCBeagle()" "\n");

	printf("crcb.search([" "\n");
	for(int i = 1 ; i < argc ; i++){
		FILE *f = fopen(argv[i], "rb");
		if(f != NULL){
			fseek(f, 0, SEEK_END);
			int length = ftell(f);
			fseek(f, 0, SEEK_SET);
			printf("[");
			unsigned char xor = 0;
			int i = length - 2; // don't include the 2 last bytes
			while(i--){
				unsigned char octet;
				fread(&octet, 1, 1, f);
				printf("%d, ", octet);
				xor ^= octet;
			}
			printf("], #xor=%d" "\n", xor);
		}
		fclose(f);
	}
	printf("]," "\n");

	printf("[" "\n");
	for(int i = 1 ; i < argc ; i++){
		FILE *f = fopen(argv[i], "rb");
		if(f != NULL){
			fseek(f, -2, SEEK_END);
			printf("[");
			int i = 2;
			while(i--){
				unsigned char octet;
				fread(&octet, 1, 1, f);
				printf("%d, ", octet);
			}
			printf("]," "\n");
		}
		fclose(f);
	}
	printf("]" "\n" ")" "\n");

	return(0);
}

