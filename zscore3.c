#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/time.h>
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>
// stb_image_write.h — tiny public-domain JPEG writer (single header)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"   // ← download from https://github.com/nothings/stb

#define RECEIVE_BUFFER_SIZE (1024)
#define SCOREBOARD_BITMAP_WIDTH (960)
#define SCOREBOARD_BITMAP_HEIGHT (128)
#define LOCAL_NAME_BITMAP_X 8
#define LOCAL_NAME_BITMAP_Y (64 - 16)
#define LOCAL_NAME_BITMAP_WIDTH 512
#define LOCAL_NAME_BITMAP_HEIGHT 60
#define VISITOR_NAME_BITMAP_X 8
#define VISITOR_NAME_BITMAP_Y (128 - 16)
#define VISITOR_NAME_BITMAP_WIDTH 512
#define VISITOR_NAME_BITMAP_HEIGHT 60

int listenSocket(struct in_addr *address, unsigned short port){
	struct sockaddr_in local_sock_addr = {.sin_family = AF_INET, .sin_addr = *address, .sin_port = port};
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	// fprintf(stderr, "listen_socket=%d" "\n", listen_socket);
	const int enable = 1;
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		// fprintf(stderr,"setsockopt(SO_REUSEADDR) failed" "\n");
	}else{
		int error = bind(listen_socket, (struct sockaddr *)&local_sock_addr, sizeof(local_sock_addr));
		if(error){
			close(listen_socket);
			listen_socket = -1;
		}
	}
	if(listen_socket >= 0){
		int error = listen(listen_socket, 1);
		if(error){
			close(listen_socket);
			listen_socket = -2;
		}
	}
	return listen_socket;
}

#define SCORE_LOCAL (0)
#define SCORE_VISITOR (1)

#define SCORE_BOARD_MAX_DATA (512)

typedef struct {
	const char *name;
	int setScores[5];
	int setWon;
	int timeouts;
	int timeout_running;
	int timeout_countdown;
	int timeoutMask;
} TeamScore;

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} pixel_s;

typedef struct {
	TeamScore local;
	TeamScore visitor;
	int serve;
	int currentSet;
	char data[SCORE_BOARD_MAX_DATA];
	int index;
	// GFX stuff
	// A pixel buffer to pass on to the JPEG encoder
	pixel_s bitmap[SCOREBOARD_BITMAP_WIDTH * SCOREBOARD_BITMAP_HEIGHT];
	// Harfbuzz and Cairo stuff
	

} ScoreBoard;

static void jpegWriteFunction(void *context, void *data, int len){
	// fprintf(stderr, "%s(len=%i)" "\n", __func__, len);
	FILE *out = (FILE*)context;
	fwrite(data, len, 1, out);
}

void scoreBoardInitBitmap(ScoreBoard *sb);

static void scoreBoardJpegOutput(ScoreBoard *sb, FILE *out){
	fprintf(stderr, "%s()" "\n", __func__);
	stbi_write_jpg_to_func(jpegWriteFunction, out, SCOREBOARD_BITMAP_WIDTH, SCOREBOARD_BITMAP_HEIGHT, 4, sb->bitmap, 85);
	fflush(out);
}

void scoreBoardDrawString(ScoreBoard *sb, const char *str, int x, int y, int w, int h){
}

void scoreBoardDrawVLine(ScoreBoard *sb, int x, int startY, int endY, pixel_s *color){
	pixel_s *pixelPtr = sb->bitmap + SCOREBOARD_BITMAP_WIDTH * startY + x;
	for(int y = startY ; y <= endY ; y++){
		*pixelPtr = *color;
		pixelPtr += SCOREBOARD_BITMAP_WIDTH;
	}
}

void scoreBoardDrawHLine(ScoreBoard *sb, int y, int startX, int endX, pixel_s *color){
	pixel_s *pixelPtr = sb->bitmap + SCOREBOARD_BITMAP_WIDTH * y + startX;
	for(int x = startX ; x <= endX ; x++){
		*pixelPtr++ = *color;
	}
}

void scoreBoardInitBitmap(ScoreBoard *sb){
	// Draw frame
	pixel_s black = {0, 0, 0, 0};
	pixel_s white = {255, 255, 255, 255};

	// "Clear" the buffer
	int i = SCOREBOARD_BITMAP_WIDTH * SCOREBOARD_BITMAP_HEIGHT;
	while(i--){
		sb->bitmap[i] = white;
	}

	scoreBoardDrawHLine(sb,   0, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb,   1, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb,  62, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb,  63, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb,  64, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb,  65, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb, 126, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	scoreBoardDrawHLine(sb, 127, 0, SCOREBOARD_BITMAP_WIDTH - 1, &black);
	
	scoreBoardDrawVLine(sb,   0, 0, SCOREBOARD_BITMAP_HEIGHT - 1, &black);
	scoreBoardDrawVLine(sb,   1, 0, SCOREBOARD_BITMAP_HEIGHT - 1, &black);

	for(int x = 478 ; x < 958 ; x += 60){
		scoreBoardDrawVLine(sb, x    , 0, SCOREBOARD_BITMAP_HEIGHT - 1, &black);
		scoreBoardDrawVLine(sb, x + 1, 0, SCOREBOARD_BITMAP_HEIGHT - 1, &black);
	}

	// scoreBoardDrawString(sb, sb->local.name,   LOCAL_NAME_BITMAP_X,   LOCAL_NAME_BITMAP_Y,   LOCAL_NAME_BITMAP_WIDTH,   LOCAL_NAME_BITMAP_HEIGHT);
	// scoreBoardDrawString(sb, sb->local.name, VISITOR_NAME_BITMAP_X, VISITOR_NAME_BITMAP_Y, VISITOR_NAME_BITMAP_WIDTH, VISITOR_NAME_BITMAP_HEIGHT);
}

void scoreBoardInit(ScoreBoard *sb, const char *local_name, const char *visitor_name, const char *fontPath){
	memset(sb, 0, sizeof(*sb));
	sb->local.name = local_name;
	sb->visitor.name = visitor_name;
	sb->serve = SCORE_LOCAL;
	sb->index = 0;
	if(NULL != fontPath){
		//scoreBoardInitHarfbuzz(sb, fontPath);
		scoreBoardInitBitmap(sb);
		scoreBoardJpegOutput(sb, stdout);
	}
}

void scoreBoardDrawRectangle(ScoreBoard *sb, int startX, int startY, int endX, int endY, pixel_s *color){
	for(int x = startX ; x <= endX ; x++){
		scoreBoardDrawVLine(sb, x, startY, endY, color);
	}
}

void scoreBoardDraw(ScoreBoard *sb){
	pixel_s blue = {0, 0, 255, 0};
	pixel_s yellow = {255, 255, 0, 0};
	scoreBoardDrawRectangle(sb, 480, 2, 480 + 60 -2, 60, (SCORE_LOCAL == sb->serve)?&blue:&yellow);
	scoreBoardDrawRectangle(sb, 480, 66, 480 + 60 -2, 124, (SCORE_LOCAL == sb->serve)?&yellow:&blue);
}

static int nibblesToValue(unsigned char nibbles){
        switch(nibbles){
                case 0x55:
                case 0xAA:
                        return(0);
                case 0x56:
                        return(1);
                case 0x59:
                        return(2);
                case 0x5A:
                        return(3);
                case 0x65:
                        return(4);
                case 0x66:
                        return(5);
                case 0x69:
                        return(6);
                case 0x6A:
                        return(7);
                case 0x95:
                        return(8);
                case 0x96:
                        return(9);
                default:
                        return(-1);
        }
}


/*
 * Examples of expected lines:
 * blablabla, F1 A5 56 55 55 55 55 95 56 55 65 55 55 55 56 55 55 56 55 55 66 55 55 55 55 55 55 55 55 65 59 55 AA 55 AA AA AA 55 55 55 AA 55 55 AA 55 AA AA AA AA AA AA AA AA 6A 59 A6 99 8F 
 */

void scoreBoardDecode(ScoreBoard *sb){
	fprintf(stderr, "%s(%s)" "\n", __func__, sb->data);
	const char *coma = strchr(sb->data, ',');
	unsigned char binaries[58];
	if(NULL != coma){
		const char *start = coma + 2;
		for(int i = 0 ; i < 58 ; i++){
			int hexa = 0;
			sscanf(start + 3 * i, "%02X", &hexa);
			binaries[i] = hexa;
		}
	}else{
		fprintf(stderr, "no coma found" "\n");
		return;
	}

#if 0
	for(int i = 0 ; i < sizeof(binaries) ; i++){
		fprintf(stderr, "[%2d]=%02X ", i, binaries[i]);
	}
	fputc('\n', stderr);
#endif
		if((0 == memcmp(binaries, "\xF1\xA5", 2)) && (0x8F == binaries[sizeof(binaries) - 1])){
			fprintf(stderr, "%s: correct sync and last byte" "\n", __func__);
			do{
#define PRINT_INT(X) printf(#X "=%d" "\n", X);
				int setValue = nibblesToValue(binaries[45 - 12]); // count set(s) already played
				if(setValue < 0){
					break;
				}

				int localScoreLSB = nibblesToValue(binaries[53 - 12]); // LSB
				if(localScoreLSB < 0){
					break;
				}

				int localScoreMSB = nibblesToValue(binaries[54 - 12]); // MSB
				if(localScoreMSB < 0){
					break;
				}
				int visitorScoreLSB = nibblesToValue(binaries[50 - 12]); // LSB
				if(visitorScoreLSB < 0){
					break;
				}
				int visitorScoreMSB = nibblesToValue(binaries[51 - 12]); // MSB
				if(visitorScoreMSB < 0){
					break;
				}

				int localTimeoutValue = nibblesToValue(binaries[32 - 12]); //  bit 0 is first TO, bit 1 second TO and bit 2 is serve
				if(localTimeoutValue < 0){
					break;
				}
				int visitorTimeoutValue = nibblesToValue(binaries[31 - 12]); // bit 0 is first TO, bit 1 second TO and bit 2 is serve
				if(visitorTimeoutValue < 0){
					break;
				}

				int localSetValue = nibblesToValue(binaries[56 - 12]); // count set(s) won by left
				if(localSetValue < 0){
					break;
				}
				int visitorSetValue = nibblesToValue(binaries[49 - 12]); // count set(s) won by right
				if(visitorSetValue < 0){
					break;
				}

				int set1Values[4];
				for(int i = 0 ; i < 4 ; i++){
					set1Values[i] = nibblesToValue(binaries[61 + i - 12]);
				}
				int set2Values[4];
				for(int i = 0 ; i < 4 ; i++){
					set2Values[i] = nibblesToValue(binaries[57 + i - 12]);
				}
				int timer[4];
				for(int i = 0 ; i < 4 ; i++){
					timer[i] = nibblesToValue(binaries[44 - i - 12]);
				}

				int left_to_mask = nibblesToValue(binaries[26 - 12]); //  bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)
				int right_to_mask = nibblesToValue(binaries[25 - 12]); // bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)

				int checkScore(int values[4], int *local, int *visitor){
					for(int i = 0 ; i < 4 ; i++){
						if(-1 == values[i]){
							return(-1);
						}
					}
					*local =   values[3] * 10 + values[2];
					*visitor = values[1] * 10 + values[0];
					return 0;
				}

				if((0 <= setValue) && (setValue < 5)){
					fprintf(stderr, "No Error Detected" "\n");
					sb->currentSet = setValue;
					int localScore = 10 * localScoreMSB + localScoreLSB;
					int visitorScore = 10 * visitorScoreMSB + visitorScoreLSB;
					sb->local.setScores[setValue] = localScore;
					sb->visitor.setScores[setValue] = visitorScore;
					sb->local.setWon = localSetValue;
					sb->visitor.setWon = visitorSetValue;
					sb->local.timeouts = localTimeoutValue;
					sb->visitor.timeouts = visitorTimeoutValue;
					sb->local.timeoutMask = left_to_mask;
					sb->visitor.timeoutMask = right_to_mask;
					if(localTimeoutValue & 4){
						sb->serve = SCORE_LOCAL;
					}
					if(visitorTimeoutValue & 4){
						sb->serve = SCORE_VISITOR;
					}
					if(0x03 & left_to_mask){
						sb->local.timeout_running = 1;
						fprintf(stderr, "local TIMEOUT" "\n");
					}else{
						sb->local.timeout_running = 0;
					}
					if(0x03 & right_to_mask){
						sb->visitor.timeout_running = 1;
						fprintf(stderr, "visitor TIMEOUT" "\n");
					}else{
						sb->visitor.timeout_running = 0;
					}
					// In case we missed some point in the 2 first sets,
					// we can get them from set1Value and set2Value
					if(setValue > 0){
						if(0 == checkScore(set1Values, &localScore, &visitorScore)){
							sb->local.setScores[0] = localScore;
							sb->visitor.setScores[0] = visitorScore;
						}
					}
					if(setValue > 1){
						if(0 == checkScore(set2Values, &localScore, &visitorScore)){
							sb->local.setScores[1] = localScore;
							sb->visitor.setScores[1] = visitorScore;
						}
					}
					fprintf(stderr, "Score:");
					for(int i = 0 ; i <= setValue ; i++){
						fprintf(stderr, " %2d/%2d", sb->local.setScores[i], sb->visitor.setScores[i]);
					}
					fprintf(stderr, "\n");
					fprintf(stderr, "TO/TOM %x/%x [%d %d : %d %d] %x/%x TOM/TO" "\n", localTimeoutValue, left_to_mask, timer[0], timer[1], timer[2], timer[3], right_to_mask, visitorTimeoutValue);
				}else{
#if 1
					for(int i = 0 ; i < sizeof(binaries) ; i++){
						fprintf(stderr, "[%2d]=%02X ", i, binaries[i]);
					}
					fputc('\n', stderr);
#endif
				}
			}while(0);
		}
}

ScoreBoard oldScoreBoard;

int scoreBoardAnalyze(ScoreBoard *sb, char *buffer, int n){
	int i = n;
	char *p = buffer;
	while(i--){
		char car = *p++;
		if('\n' == car){
			if(strcmp(oldScoreBoard.data, sb->data)){
				// fprintf(stderr, "change detected => redrawing" "\n" "was: %s" "\n" "now: %s" "\n", oldScoreBoard.data, sb->data);
				scoreBoardDecode(sb);
				scoreBoardDraw(sb);
				scoreBoardJpegOutput(sb, stdout);
				strcpy(oldScoreBoard.data, sb->data);
			}
			sb->index = 0;
		}else{
			if(sb->index < (SCORE_BOARD_MAX_DATA - 1)){
				sb->data[sb->index] = car;
				sb->index++;
				sb->data[sb->index] = '\0';
			}
		}
	}
	return(sb->index);
}

int main(int argc, char * const argv[]){

	unsigned short scorePort = htons(8366); // listen to this port for incoming score board connections
	struct in_addr scoreAddress = {0}; // bind to this address for score connections

	int trafficTimeout = 30;

	const char *locaux = strdup("LOCAUX");
	const char *visiteurs = strdup("VISITEURS");

	char *recvBuffer = (char *)malloc(RECEIVE_BUFFER_SIZE);

	ScoreBoard scoreBoard;

	const char *fontPath = "/usr/share/fonts/truetype/liberation/LiberationSansNarrow-Bold.ttf";

        while (1) {
                int option_index = 0;
                static struct option long_options[] = {
			{"score",    required_argument, 0,  's' }, // ScoreBoard info listen port
			{"address",  required_argument, 0,  'a' }, // optional local IPv4 address to bind to for the scorer data
			{"timeout",  required_argument, 0,  't' }, // no transfer timeout
			{"local",    required_argument, 0,  'l' }, // name of local team
			{"visitor",  required_argument, 0,  'v' }, // name of visiting team
			{"font",     required_argument, 0,  'f' }, // font path
			{0,          0,                 0,  0 }
                };

                int c = getopt_long(argc, argv, "s:a:t:l:v:", long_options, &option_index);
                if (-1 == c){
			break;
		}

                switch (c) {
                        case 's':
				scorePort = htons(atoi(optarg));
                        break;
                        case 'a':
				inet_aton(optarg, &scoreAddress);
                        break;
                        case 't':
				trafficTimeout = atoi(optarg);
                        break;
			case 'l':
				free((void*)locaux);
				locaux = strdup(optarg);
			break;
			case 'v':
				free((void*)visiteurs);
				visiteurs = strdup(optarg);
			break;
                        case 'f':
				fontPath = strdup(optarg);
                        break;
			default:
			break;

		}
	}
	fprintf(stderr, "Listen to ScoreBoard connection on %s:%d" "\n", inet_ntoa(scoreAddress), ntohs(scorePort));
	fprintf(stderr, "Transfert timeout (on scoreboard side):%d" "\n", trafficTimeout);
	fprintf(stderr, "Locaux=   [%s]" "\n", locaux);
	fprintf(stderr, "Visiteurs=[%s]" "\n", visiteurs);

	scoreBoardInit(&scoreBoard, locaux, visiteurs, fontPath);
	scoreBoardInit(&oldScoreBoard, NULL, NULL, NULL); // will never draw using this scoreboard

	int scoreListenSocket = listenSocket(&scoreAddress, scorePort);
	int scoreTrafficSocket = -1;

	void updateMax(int *m, int n){
		int max = *m;
		if(n > max){
			*m = n;
		}
	}

	struct timeval lastScoreTraffic;

	for(;;){
		int max = -1;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds); updateMax(&max, STDIN_FILENO);
		if(-1 != scoreListenSocket){
			FD_SET(scoreListenSocket, &fds); updateMax(&max, scoreListenSocket);
		}
		if(-1 != scoreTrafficSocket){
			FD_SET(scoreTrafficSocket, &fds); updateMax(&max, scoreTrafficSocket);
		}
		struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
		int selected = select(max + 1, &fds, NULL, NULL, &timeout);
		if(selected > 0){
			if((-1 != scoreListenSocket) && FD_ISSET(scoreListenSocket, &fds)){
				// fprintf(stderr, "Connection on scoreSocket" "\n");
				scoreTrafficSocket = accept(scoreListenSocket, NULL, NULL);
				gettimeofday(&lastScoreTraffic, NULL);
				close(scoreListenSocket);
				scoreListenSocket = -1;
			}
			if((-1 != scoreTrafficSocket) && FD_ISSET(scoreTrafficSocket, &fds)){
				gettimeofday(&lastScoreTraffic, NULL);
				int lus = recv(scoreTrafficSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
				// fprintf(stderr, "Traffic coming on scoreTrafficSocket, %d byte(s) received" "\n", lus);
				if(lus <= 0){
					close(scoreTrafficSocket);
					scoreTrafficSocket = -1;
					scoreListenSocket = listenSocket(&scoreAddress, scorePort);
				}else{
					scoreBoardAnalyze(&scoreBoard, recvBuffer, lus);
				}
			}
			if(FD_ISSET(STDIN_FILENO, &fds)){
				// fprintf(stderr, "STDIN_FILENO" "\n");
				char c = '\0';
				if(read(STDIN_FILENO, &c, sizeof(c)) > 0){
					if('q' == c){
						break;
					}
				}
			}
		}else{
			scoreBoardJpegOutput(&scoreBoard, stdout);
		}
		if(-1 != scoreTrafficSocket){
			struct timeval currentTime;
			gettimeofday(&currentTime, NULL);
			time_t delta_t = currentTime.tv_sec - lastScoreTraffic.tv_sec;
			if(delta_t > trafficTimeout){
				fprintf(stderr, "Traffic timeout expired: %lds idle" "\n", delta_t);
				close(scoreTrafficSocket);
				scoreTrafficSocket = -1;
				scoreListenSocket = listenSocket(&scoreAddress, scorePort);
			}
		}
	}

	if(scoreTrafficSocket >= 0){
		close(scoreTrafficSocket);
	}
	if(scoreListenSocket >= 0){
		close(scoreListenSocket);
	}

	if(NULL != recvBuffer){
		free(recvBuffer);
	}
	if(locaux != NULL){
		free((void*)locaux);
	}
	if(visiteurs != NULL){
		free((void*)visiteurs);
	}
	return(0);
}


