#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned char revertByte(unsigned char octet){
	// fprintf(stderr, "%s(%02X)", __func__, octet);
	unsigned char result = 0;
	for(int i = 0 ; i < 8 ; i++){
		result <<= 1;
		result |= (octet & 1);
		octet >>= 1;
	}
	// fprintf(stderr, "=>%02X" "\n", result);
	return(result);
}

int main(int argc, const char *argv[]){
	char ligne[1024];
	while(fgets(ligne, sizeof(ligne) - 1, stdin)){
		char *coma = strchr(ligne, ',');
		if(coma){
			char *p = coma + 2;
			for(;;){
				unsigned int hexa = 0;
				if(sscanf(p, "%02X", &hexa) && (0 != hexa)){
					// fprintf(stderr, "hexa=%02X" "\n", hexa);
					unsigned char octet = (unsigned char)hexa;
					octet = revertByte(octet);
					char revertByteString[4];
					snprintf(revertByteString, 3, "%02X", octet);
					memcpy(p, revertByteString, 2);
				}else{
					break;
				}
				p += 3;
			}
		}else{
			break;
		}
		fputs(ligne, stdout);
	}
	return(0);
}



