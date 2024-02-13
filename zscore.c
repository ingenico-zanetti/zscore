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

#include "ballon.h"

#define RECEIVE_BUFFER_SIZE (1024)

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

typedef enum {
	HTTP_REQUEST_STATE_WAIT_CR_1 = 0,
	HTTP_REQUEST_STATE_WAIT_LF_1 = 1,
	HTTP_REQUEST_STATE_WAIT_CR_2 = 2,
	HTTP_REQUEST_STATE_WAIT_LF_2 = 3,
	HTTP_REQUEST_STATE_END = 5
} HttpRequestState;

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
} TeamScore;

typedef struct {
	TeamScore local;
	TeamScore visitor;
	int serve;
	int currentSet;
	char data[SCORE_BOARD_MAX_DATA];
	int index;
} ScoreBoard;

void scoreBoardInit(ScoreBoard *sb, const char *local_name, const char *visitor_name){
	memset(sb, 0, sizeof(*sb));
	sb->local.name = local_name;
	sb->visitor.name = visitor_name;
	sb->serve = SCORE_LOCAL;
	sb->index = 0;
	memset(sb->data, 0, sizeof(sb->data));
#ifdef __TEST_SCORE_MAX__
	sb->currentSet = 4;
	for(int i = 0 ; i < 5 ; i++){
		sb->local.setScores[i] = 25;
		sb->visitor.setScores[i] = 25;
	}
#endif
}

#if 0
static int nibblesToDigit(unsigned char nibbles){
        switch(nibbles){
                case 0x55:
                        return('0');
                case 0x56:
                        return('1');
                case 0x59:
                        return('2');
                case 0x5A:
                        return('3');
                case 0x65:
                        return('4');
                case 0x66:
                        return('5');
                case 0x69:
                        return('6');
                case 0x6A:
                        return('7');
                case 0x95:
                        return('8');
                case 0x96:
                        return('9');
                default:
                        return(' ');
        }
}
#endif

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

       // fprintf(stdout, "st[%c %c%c%c:%c%c%c %c] [%c] sc[%c%c %c%c] 1[%c%c:%c%c] 2[%c%c:%c%c] TO[%c %c]", left_set_digit, (left_to_mask & 0x03) ? 'T' : ' ', timer[0], timer[1], timer[2], timer[3], (right_to_mask & 0x03) ? 'T' : ' ',right_set_digit, set_digit, left_score_digit_1, left_score_digit_0, right_score_digit_1, right_score_digit_0, set1[3], set1[2], set1[1], set1[0], set2[3], set2[2], set2[1], set2[0], left_to_digit, right_to_digit);

void scoreBoardDecode(ScoreBoard *sb){
	// look for '.', then ":"
	if((sb->index > (70 * 3 + 21)) && ('.' == sb->data[10]) && (':' == sb->data[19])){
		unsigned char binaries[70];
		char *p = sb->data + 21;
		for(int i = 0 ; i < sizeof(binaries) ; i++){
			unsigned int hexa;
			sscanf(sb->data + 21 + 3 * i, "%02X", &hexa);
			binaries[i] = (unsigned char)hexa;
		}
		if((0 == memcmp(binaries, "\xE5\x95\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\xF1\xA5", 14)) && (0x8F == binaries[70 - 1])){
			do{
#define PRINT_INT(X) printf(#X "=%d" "\n", X);
				int setValue = nibblesToValue(binaries[45]); // count set(s) already played
				if(setValue < 0){
					break;
				}

				int localScoreLSB = nibblesToValue(binaries[53]); // LSB
				if(localScoreLSB < 0){
					break;
				}

				int localScoreMSB = nibblesToValue(binaries[54]); // MSB
				if(localScoreMSB < 0){
					break;
				}
				int visitorScoreLSB = nibblesToValue(binaries[50]); // LSB
				if(visitorScoreLSB < 0){
					break;
				}
				int visitorScoreMSB = nibblesToValue(binaries[51]); // MSB
				if(visitorScoreMSB < 0){
					break;
				}

				int localTimeoutValue = nibblesToValue(binaries[32]); //  bit 0 is first TO, bit 1 second TO and bit 2 is serve
				if(localTimeoutValue < 0){
					break;
				}
				int visitorTimeoutValue = nibblesToValue(binaries[31]); // bit 0 is first TO, bit 1 second TO and bit 2 is serve
				if(visitorTimeoutValue < 0){
					break;
				}

				int localSetValue = nibblesToValue(binaries[56]); // count set(s) won by left
				if(localSetValue < 0){
					break;
				}
				int visitorSetValue = nibblesToValue(binaries[49]); // count set(s) won by right
				if(visitorSetValue < 0){
					break;
				}

				int set1Values[4];
				for(int i = 0 ; i < 4 ; i++){
					set1Values[i] = nibblesToValue(binaries[61 + i]);
				}
				int set2Values[4];
				for(int i = 0 ; i < 4 ; i++){
					set2Values[i] = nibblesToValue(binaries[57 + i]);
				}
#if 0
				int timer[4];
				for(int i = 0 ; i < 4 ; i++){
					timer[i] = nibblesToValue(binaries[44 - i]);
				}
				int left_to_mask = nibblesToValue(binaries[26]); //  bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)
				int right_to_mask = nibblesToValue(binaries[25]); // bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)

#endif

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
					if(localTimeoutValue & 4){
						sb->serve = SCORE_LOCAL;
					}
					if(visitorTimeoutValue & 4){
						sb->serve = SCORE_VISITOR;
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
}

void scoreBoardAnalyze(ScoreBoard *sb, char *buffer, int n){
	int i = n;
	char *p = buffer;
	while(i--){
		char car = *p++;
		if('\n' == car){
			scoreBoardDecode(sb);
			sb->index = 0;
		}else{
			if(sb->index < (SCORE_BOARD_MAX_DATA - 1)){
				sb->data[sb->index] = car;
				sb->index++;
				sb->data[sb->index] = '\0';
			}
		}
	}
}

#define HTTP_REQUEST_MAX_FIRST_LINE_LENGTH (4096)

typedef struct {
	HttpRequestState state;
	char firstLine[HTTP_REQUEST_MAX_FIRST_LINE_LENGTH];
	int firstLineLength;
	int isFirstLine;
	int socketFd;
	ScoreBoard *scoreBoard;
} HttpRequest;

static void httpRequestReinit(HttpRequest *h){
	h->state = HTTP_REQUEST_STATE_WAIT_CR_1;
	memset(h->firstLine, 0, sizeof(h->firstLine));
	h->firstLineLength = 0;
	h->isFirstLine = 1;
}

static void httpRequestInit(HttpRequest *h, int fd, ScoreBoard *s){
	httpRequestReinit(h);
	h->socketFd = fd;
	h->scoreBoard = s;
}

int sendLine(int socketFd, const char *ligne){
	const char *crlf = "\r\n";
	int len = strlen(ligne);
	int total = 0;
	if(len > 0){
		total += send(socketFd, ligne, len, 0);
	}
	// fprintf(stdout, "%s" "\n", ligne);
	total += send(socketFd, crlf, strlen(crlf), 0);
	return total;
}

static void httpRequestAnswerZscore(HttpRequest *h){
	void tableRow(int s, int hasServe, TeamScore *ts, int currentSet){
		char ligne[128];
		sprintf(ligne, "<TR>"); 
		sendLine(s, ligne);
		sprintf(ligne, "<TD bgcolor='#404040' style='color:white'>%.32s</TD>", ts->name);
		sendLine(s, ligne);
		if(hasServe){
			sprintf(ligne, "<TD bgcolor='#FFFFFF'><IMG src='ballon.png'></TD>");
		}else{
			sprintf(ligne, "<TD bgcolor='#404040'>" "</TD>");
		}
		sendLine(s, ligne);
		sprintf(ligne, "<TD bgcolor='#4040E0' style='color:white'>%d</TD>", ts->setWon);
		sendLine(s, ligne);
		for(int i = 0 ; i <= currentSet ; i++){
			sprintf(ligne, "<TD bgcolor='#404040' style='color:%s'>%02d</TD>", (i == currentSet) ? "#FFFFFF" : "#C0C0C0", ts->setScores[i]);
			sendLine(s, ligne);
		}
		sendLine(s, "</TR>");
	}

	sendLine(h->socketFd, "HTTP/1.0 200 OK");
	sendLine(h->socketFd, "Content-Type: text/html");
	sendLine(h->socketFd, "Accept-Ranges: bytes");

	char lastModified[64];
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);

	strftime(lastModified, sizeof(lastModified) - 1, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT", tmp);
	sendLine(h->socketFd, lastModified);

	sendLine(h->socketFd, "Connection: close");

	strftime(lastModified, sizeof(lastModified) - 1, "Date: %a, %d %b %Y %H:%M:%S GMT", tmp);
	sendLine(h->socketFd, lastModified);

	sendLine(h->socketFd, "Server: zscore/1.0.0");
	sendLine(h->socketFd, "");

	sendLine(h->socketFd, "<html>");
	sendLine(h->socketFd, "	<head>");
	sendLine(h->socketFd, "		<meta http-equiv=\"refresh\" content=\"3\">");
	sendLine(h->socketFd, "		<meta charset=\"UTF-8\">");

	sendLine(h->socketFd, "<style>");
	sendLine(h->socketFd, "td {");
	sendLine(h->socketFd, "font-family: courier-new;");
	sendLine(h->socketFd, "font-size: 36px;");
	sendLine(h->socketFd, "font-weight: bold;");
	sendLine(h->socketFd, "border: 0px solid black;");
	sendLine(h->socketFd, "padding: 8px;");
	sendLine(h->socketFd, "}");
	sendLine(h->socketFd, "</style>");
	char title[128];

	sprintf(title, "<title>%.32s contre %.32s</title>", h->scoreBoard->local.name, h->scoreBoard->visitor.name);
	sendLine(h->socketFd, title);

	sendLine(h->socketFd, "	</head>");
	sendLine(h->socketFd, "	<body>");

	sendLine(h->socketFd, "<TABLE BORDER='0'>");
	int actualCurrentSet = h->scoreBoard->currentSet;
	if(actualCurrentSet > 4){
		actualCurrentSet = 4;
	}
	tableRow(h->socketFd, h->scoreBoard->serve == SCORE_LOCAL, &(h->scoreBoard->local),   actualCurrentSet);
	tableRow(h->socketFd, h->scoreBoard->serve != SCORE_LOCAL, &(h->scoreBoard->visitor), actualCurrentSet);
	sendLine(h->socketFd, "</TABLE>");
	sendLine(h->socketFd, "	</body>");
	sendLine(h->socketFd, "</html>");
	sendLine(h->socketFd, "");

}

static void httpRequestAnswerBallon(HttpRequest *h){
	char ligne[128];
	sendLine(h->socketFd, "HTTP/1.0 200 OK");
	sendLine(h->socketFd, "Content-Type: image/png");
	sendLine(h->socketFd, "Accept-Ranges: bytes");
	sprintf(ligne, "Content-Length: %d", ballon_png_data_len);
	sendLine(h->socketFd, ligne);
	sendLine(h->socketFd, "");

	send(h->socketFd, ballon_png_data, ballon_png_data_len, 0);
}

#define BALLON_PNG_STRING "GET /ballon.png"
#define FAVICON_ICO_STRING "GET /favicon.ico"

static void httpRequestAnswer(HttpRequest *h){
	if((0 == strncmp(BALLON_PNG_STRING, h->firstLine, sizeof(BALLON_PNG_STRING) - 1))  || (0 == strncmp(FAVICON_ICO_STRING, h->firstLine, sizeof(FAVICON_ICO_STRING) - 1))){
		fprintf(stderr, "ballon.png or favicon.ico requested, sending ballon.png" "\n");
		httpRequestAnswerBallon(h);
	}else{
		httpRequestAnswerZscore(h);
	}
}

static int httpRequestUpdate(HttpRequest *h, char *buffer, int len, unsigned short httpPort){
	int returnValue = 0;
	char *p = buffer;
	int i = len;
	while(i--){
		char c = *p++;
		switch(c){
			case '\r':
				h->isFirstLine = 0;
				switch(h->state){
					case HTTP_REQUEST_STATE_WAIT_CR_1:
					case HTTP_REQUEST_STATE_WAIT_CR_2:
						h->state++;
					break;
					default:
						h->state = HTTP_REQUEST_STATE_WAIT_CR_1;
					break;
				}
				break;
			case '\n':
				h->isFirstLine = 0;
				if(HTTP_REQUEST_STATE_WAIT_LF_1 == h->state){
					h->state++;
				}
				if(HTTP_REQUEST_STATE_WAIT_LF_2 == h->state){
					fprintf(stderr, "h->firstLine=[%s]" "\n", h->firstLine);
					httpRequestAnswer(h);
					httpRequestReinit(h);
					returnValue = 1;
				}
				break;
			default:
				if(h->isFirstLine){
					if(h->firstLineLength < (HTTP_REQUEST_MAX_FIRST_LINE_LENGTH - 1)){
						h->firstLine[h->firstLineLength++] = c;
					}
				}
				h->state = HTTP_REQUEST_STATE_WAIT_CR_1;
			break;
		}
		fputc(c, stderr);
	}
	return returnValue;
}

int main(int argc, char * const argv[]){

	unsigned short scorePort = 8366; // listen to this port for incoming score board connections
	struct in_addr scoreAddress = {0}; // bind to this address for score connections

	unsigned short httpPort = 8080;  // listen to this port for incoming HTTP connections
	struct in_addr httpAddress = {0}; // bind to this address for HTTP connections

	int trafficTimeout = 30;
	int updateDelay = 10; // second(s) between we receive a new score and it reflects on HTTP size

	const char *locaux = strdup("LOCAUX");
	const char *visiteurs = strdup("VISITEURS");

	char *recvBuffer = (char *)malloc(RECEIVE_BUFFER_SIZE);

	ScoreBoard scoreBoard;

        while (1) {
                int option_index = 0;
                static struct option long_options[] = {
			{"bind",     required_argument, 0,  'b' }, // optional local IPv4 address to bind to for HTTP connection
			{"http",     required_argument, 0,  'h' }, // HTTP listen TCP port
			{"score",    required_argument, 0,  's' }, // ScoreBoard info listen port
			{"address",  required_argument, 0,  'a' }, // optional local IPv4 address to bind to for HTTP connection
			{"timeout",  required_argument, 0,  't' }, // no transfer timeout
			{"delay",    required_argument, 0,  'd' }, // delay from receiving score to updating HTTP side
			{"local",    required_argument, 0,  'l' }, // name of local team
			{"visitor",  required_argument, 0,  'v' }, // name of visiting team
			{0,          0,                 0,  0 }
                };

                int c = getopt_long(argc, argv, "b:h:s:a:t:d:l:v:", long_options, &option_index);
                if (-1 == c){
			break;
		}

                switch (c) {
                        case 'b':
				inet_aton(optarg, &httpAddress);
                        break;
                        case 'a':
				inet_aton(optarg, &scoreAddress);
                        break;
                        case 'h':
				httpPort = htons(atoi(optarg));
                        break;
                        case 's':
				scorePort = htons(atoi(optarg));
                        break;
                        case 't':
				trafficTimeout = atoi(optarg);
                        break;
                        case 'd':
				updateDelay = atoi(optarg);
                        break;
			case 'l':
				free((void*)locaux);
				locaux = strdup(optarg);
			break;
			case 'v':
				free((void*)visiteurs);
				visiteurs = strdup(optarg);
			break;

		}
	}
	fprintf(stderr, "Listen to ScoreBoard connection on %s:%d" "\n", inet_ntoa(scoreAddress), ntohs(scorePort));
	fprintf(stderr, "Listen to       HTTP connection on %s:%d" "\n", inet_ntoa(httpAddress),  ntohs(httpPort));
	fprintf(stderr, "Transfert timeout (on scorebaord side):%d" "\n", trafficTimeout);
	fprintf(stderr, "Update delay (from scoreboard to HTTP):%d" "\n", updateDelay);
	fprintf(stderr, "Locaux=   [%s]" "\n", locaux);
	fprintf(stderr, "Visiteurs=[%s]" "\n", visiteurs);

	scoreBoardInit(&scoreBoard, locaux, visiteurs);

	int scoreListenSocket = listenSocket(&scoreAddress, scorePort);
	int httpListenSocket =  listenSocket(&httpAddress,  httpPort);
	int scoreTrafficSocket = -1;
	int httpTrafficSocket = -1;
	HttpRequest httpRequest;

	httpRequestInit(&httpRequest, -1, &scoreBoard);

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
		if(-1 != httpListenSocket){
			FD_SET(httpListenSocket, &fds); updateMax(&max, httpListenSocket);
		}
		if(-1 != httpTrafficSocket){
			FD_SET(httpTrafficSocket, &fds); updateMax(&max, httpTrafficSocket);
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
			if((-1 != httpListenSocket) && FD_ISSET(httpListenSocket, &fds)){
				// fprintf(stderr, "Connection on httpSocket" "\n");
				httpTrafficSocket = accept(httpListenSocket, NULL, NULL);
				close(httpListenSocket);
				httpListenSocket = -1;
				httpRequestInit(&httpRequest, httpTrafficSocket, &scoreBoard);
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
			if((-1 != httpTrafficSocket) && FD_ISSET(httpTrafficSocket, &fds)){
				int lus = recv(httpTrafficSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
				// fprintf(stderr, "Traffic coming on httpTrafficSocket, %d byte(s) received" "\n", lus);
				if((lus <= 0) || (httpRequestUpdate(&httpRequest, recvBuffer, lus, httpPort) > 0)){
					close(httpTrafficSocket);
					httpTrafficSocket = -1;
					httpListenSocket =  listenSocket(&httpAddress,  httpPort);
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
				switch(c){
					case 's':
						scoreBoard.serve ^= 1;
					break;
					case '+':
						if(SCORE_LOCAL == scoreBoard.serve){
							scoreBoard.local.setScores[scoreBoard.currentSet]++;
						}else{
							scoreBoard.visitor.setScores[scoreBoard.currentSet]++;
						}
					break;
					case '-':
						if(SCORE_LOCAL == scoreBoard.serve){
							scoreBoard.local.setScores[scoreBoard.currentSet]--;
						}else{
							scoreBoard.visitor.setScores[scoreBoard.currentSet]--;
						}
					break;
					case 'v':
						if(SCORE_LOCAL == scoreBoard.serve){
							scoreBoard.local.setWon++;
						}else{
							scoreBoard.visitor.setWon++;
						}
						scoreBoard.currentSet++;
					break;
					case 'r':
						scoreBoardInit(&scoreBoard, locaux, visiteurs);
					break;
				}

			}
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
	if(httpTrafficSocket >= 0){
		close(httpTrafficSocket);
	}
	if(scoreListenSocket >= 0){
		close(scoreListenSocket);
	}
	if(httpListenSocket >= 0){
		close(httpListenSocket);
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


