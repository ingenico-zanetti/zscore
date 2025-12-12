#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
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
	char setScoreDigits[5][2];
	int setWon;
	int timeouts;
	int timeout_running;
	int timeout_countdown;
	int timeoutMask;
} TeamScore;

#include "types.h"
#include "v200w_60x60_pixels.h"

static pixel_s black = {0, 0, 0, 0};
static pixel_s blue = {0, 0, 255, 0};
static pixel_s yellow = {255, 255, 0, 0};
static pixel_s white = {255, 255, 255, 0};
static pixel_s backgroundColor = {0x40, 0x40, 0x40, 0};
static pixel_s oldSetColor = {0xC0, 0xC0, 0xC0, 0};

typedef struct {
	TeamScore local;
	TeamScore visitor;
	int serve;
	int currentSet;
	char data[SCORE_BOARD_MAX_DATA];
	int index;
	int timers[4];
	// GFX stuff
	// A pixel buffer to pass on to the JPEG encoder
	pixel_s bitmap[SCOREBOARD_BITMAP_WIDTH * SCOREBOARD_BITMAP_HEIGHT];
	// Harfbuzz and Cairo stuff
	FT_Library ft;
	FT_Face face;
	int fontSizePixel;
	hb_font_t *hb_font;
	hb_buffer_t *buf;
} ScoreBoard;

static void jpegWriteFunction(void *context, void *data, int len){
	// fprintf(stderr, "%s(len=%i)" "\n", __func__, len);
	FILE *out = (FILE*)context;
	fwrite(data, len, 1, out);
}

void scoreBoardInitBitmap(ScoreBoard *sb);

static void scoreBoardJpegOutput(ScoreBoard *sb, FILE *out){
	// fprintf(stderr, "%s()" "\n", __func__);
	stbi_write_jpg_to_func(jpegWriteFunction, out, SCOREBOARD_BITMAP_WIDTH, SCOREBOARD_BITMAP_HEIGHT, 4, sb->bitmap, 100);
	fflush(out);
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

void scoreBoardFillRectangle(ScoreBoard *sb, int startX, int startY, int endX, int endY, pixel_s *color){
	for(int y = startY ; y <= endY ; y++){
		scoreBoardDrawHLine(sb, y, startX, endX, color);
	}
}

void scoreBoardDrawString(ScoreBoard *sb, const char *string, int baseline, int minX, int minY, int maxX, int maxY, pixel_s *color){
	hb_buffer_reset(sb->buf);
        hb_buffer_add_utf8(sb->buf, string, -1, 0, -1);
        hb_buffer_set_direction(sb->buf, HB_DIRECTION_LTR);
        hb_buffer_set_script(sb->buf, HB_SCRIPT_COMMON);
        hb_buffer_set_language(sb->buf, hb_language_from_string("en", -1));
        hb_shape(sb->hb_font, sb->buf, NULL, 0);

        unsigned int glyph_count;
        hb_glyph_info_t     *info = hb_buffer_get_glyph_infos(sb->buf, &glyph_count);
        hb_glyph_position_t *pos  = hb_buffer_get_glyph_positions(sb->buf, &glyph_count);
        
	int pen_x = minX;
        int pen_y = baseline;

        for (unsigned int i = 0; i < glyph_count; i++) {
            hb_codepoint_t gid = info[i].codepoint;
            int x_offset  = pos[i].x_offset / 64;
            int y_offset  = pos[i].y_offset / 64;
            int x_advance = pos[i].x_advance / 64;

            if (FT_Load_Glyph(sb->face, gid, FT_LOAD_RENDER)) continue;

            FT_Bitmap *bmp = &sb->face->glyph->bitmap;
            int gx = pen_x + x_offset + sb->face->glyph->bitmap_left;
            int gy = pen_y - y_offset - sb->face->glyph->bitmap_top;

            for (int y = 0; y < bmp->rows; y++) {
                for (int x = 0; x < bmp->width; x++) {
                    int px = gx + x;
                    int py = gy + y;
                    if (px < minX || px >= maxX || py < minY || py >= maxY) continue;

		    unsigned char alpha = bmp->buffer[y * bmp->pitch + x];
                    if (alpha == 0) continue;

                    sb->bitmap[(py * SCOREBOARD_BITMAP_WIDTH + px)] = *color;
                }
            }
            pen_x += x_advance;
        }
}

// Initialized once at startup
void scoreBoardInitBitmap(ScoreBoard *sb){
	// Draw frame
	// "Clear" the buffer
	int i = SCOREBOARD_BITMAP_WIDTH * SCOREBOARD_BITMAP_HEIGHT;
	while(i--){
		sb->bitmap[i] = black;
	}

	for(int y = 2 ; y < 128; y += 64){
		scoreBoardFillRectangle(sb, 2, y, (960 - (7 * 64) - 2), y + 59, &backgroundColor);
		for(int x = (960 - 7 * 64) + 2 ; x < 960 ; x += 64){
			scoreBoardFillRectangle(sb, x, y, x + 59, y + 59, &backgroundColor);
		}
	}

	scoreBoardDrawString(sb, sb->local.name, 45, 3,   3,   960 - 7 * 64,   61, &white);
	scoreBoardDrawString(sb, sb->visitor.name, 45 + 64, 3,   3 + 64,   960 - 7 * 64,   61 + 64, &white);
}

void scoreBoardInitHarfbuzz(ScoreBoard *sb, const char *fontPath){
	sb->fontSizePixel = 30;
	int dpi = 96;
	FT_Init_FreeType(&sb->ft);
	FT_New_Face(sb->ft, fontPath, 0, &sb->face);
	FT_Set_Char_Size(sb->face, 0, sb->fontSizePixel * 64, dpi, dpi);
	sb->hb_font = hb_ft_font_create(sb->face, NULL);
	sb->buf = hb_buffer_create();
}

void scoreBoardInit(ScoreBoard *sb, const char *local_name, const char *visitor_name, const char *fontPath){
	memset(sb, 0, sizeof(*sb));
	sb->local.name = local_name;
	sb->visitor.name = visitor_name;
	sb->serve = SCORE_LOCAL;
	sb->index = 0;
	if(NULL != fontPath){
		scoreBoardInitHarfbuzz(sb, fontPath);
		scoreBoardInitBitmap(sb);
		scoreBoardJpegOutput(sb, stdout);
	}
	if(0){
		const pixel_s *p = bitmap_v200w_60x60;
		fprintf(stderr, "const pixel_s bitmap_v200w_60x60_with_alpha[3600] = {" "\n");
		for(int y = 0 ; y < 60 ; y++){
			for(int x = 0 ; x < 60 ; x++){
				pixel_s pixel = *p++;
				if(pixel.red > 200 && pixel.green > 200 && pixel.blue > 200){
					pixel.alpha = 0;
				}else{
					pixel.alpha = 255;
				}
				fprintf(stderr, "{ 0x%02X, 0x%02X, 0x%02X, 0x%02X },", pixel.red, pixel.green, pixel.blue, pixel.alpha);
			}
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n" "};" "\n");
		exit(0);
	}
}

void scoreBoardCopyBitmap(ScoreBoard *sb, int x, int y, const pixel_s *bitmap, int bitmapWidth, int bitmapHeight, bool useAlpha){
	pixel_s *dst = sb->bitmap + (x + y * SCOREBOARD_BITMAP_WIDTH);
	const pixel_s *src = bitmap;
	for(int i = 0 ; i < bitmapHeight ; i++){
		if(useAlpha){
			if(1){
			const pixel_s *s = src;
			pixel_s *d = dst;
			for(int x = 0 ; x < bitmapWidth ; x++){
				pixel_s pixel = *s++;
				if(pixel.alpha){
					*d = pixel;
				}
				d++;
			}
			}
		}else{
			memcpy(dst, src, sizeof(pixel_s) * bitmapWidth);
		}
		dst += SCOREBOARD_BITMAP_WIDTH;
		src += bitmapWidth;
	}
}

void scoreBoardDrawTeamScore(ScoreBoard *sb, TeamScore *ts, int baseline, const int currentSet){
	for(int set = 0 ; set < 5 ; set++){
		char str[4];
		bool drawMSB = false;
		bool drawLSB = false;
		if(ts->setScoreDigits[set][0]){
			drawMSB = true;
			drawLSB = true;
		}else if(ts->setScoreDigits[set][1]){
			drawLSB = true;
		}
		pixel_s *color = &oldSetColor;
		if(currentSet == set){
			color = &white;
		}
		// fprintf(stderr, "set %i, local, value=%d%d, drawMSB=%i, drawLSB=%i" "\n", set, ts->setScoreDigits[set][0], ts->setScoreDigits[set][1], drawMSB, drawLSB);
		if(drawMSB){
			str[0] = '0' + ts->setScoreDigits[set][0];
			str[1] = '\0';
			int startX = 960 - 6 * 64 + set * 64;
			scoreBoardDrawString(sb, str, baseline, startX +  4, 2, startX + 59,  126, color);
		}
		if(drawLSB){
			str[0] = '0' + ts->setScoreDigits[set][1];
			str[1] = '\0';
			int startX = 960 - 6 * 64 + set * 64;
			scoreBoardDrawString(sb, str, baseline, startX + 34, 2, startX + 59,  126, color);
		}
	}
}

void scoreBoardDraw(ScoreBoard *sb){
	// Who has serve ?
	if(SCORE_LOCAL == sb->serve){
		scoreBoardCopyBitmap(sb, 960 - (7 * 64 - 2),  2, bitmap_v200w_60x60, 60, 60, true);
		scoreBoardFillRectangle(sb, 960 - (7 * 64 - 2), 66, 960 - (7 * 64 - 2) + 59, 66 + 59, &backgroundColor);
	}else{
		scoreBoardCopyBitmap(sb, 960 - (7 * 64 - 2), 66, bitmap_v200w_60x60, 60, 60, true);
		scoreBoardFillRectangle(sb, 960 - (7 * 64 - 2), 2, 960 - (7 * 64 - 2) + 59, 2 + 59,   &backgroundColor);
	}

	// Set scores
	for(int y = 2 ; y < 128; y += 64){
		for(int x = (960 - 6 * 64) + 2 ; x < 960 ; x += 64){
			scoreBoardFillRectangle(sb, x, y, x + 59, y + 59, &backgroundColor);
		}
	}
	scoreBoardDrawTeamScore(sb, &sb->local, 45, sb->currentSet);
	scoreBoardDrawTeamScore(sb, &sb->visitor, 45 + 64, sb->currentSet);

	// What about timeouts ?
	int left_timeout = sb->local.timeouts & 3;
	int right_timeout = sb->visitor.timeouts & 3;
	if((0 != sb->timers[2]) || (0 != sb->timers[3])){
		// Running timeout
		if(1 & sb->timers[3]){ // blink every second
			left_timeout ^= sb->local.timeoutMask;
			right_timeout ^= sb->visitor.timeoutMask;
		}
	}
	// fprintf(stderr, "%d %d" "\n", left_timeout, right_timeout);
	int startX = 960 - 64 + 2;
	scoreBoardFillRectangle(sb, startX,  2, startX + 59,  2 + 59, &backgroundColor);
	scoreBoardFillRectangle(sb, startX, 66, startX + 59, 66 + 59, &backgroundColor);
	int mask = 1;
	for(int i = 0 ; i < 2 ; i++){
		scoreBoardDrawString(sb, (left_timeout & mask) ?  "\u25CF" : "\u25EF", 45,      startX + 2 + 32 * i,       2, startX + 59,  2 + 59, &black);
		scoreBoardDrawString(sb, (right_timeout & mask) ? "\u25CF" : "\u25EF", 45 + 64, startX + 2 + 32 * i,  2 + 64, startX + 59, 64 + 59, &black);
		mask <<= 1;
	}
}

// Now use "unreflected" values
static int nibblesToValue(unsigned char nibbles){
        switch(nibbles){
                case 0x55:
                case 0xAA:
                        return(0);
		case 0x6A: // case 0x56:
                        return(1);
		case 0x9A: // case 0x59:
                        return(2);
		case 0x5A: // case 0x5A:
                        return(3);
		case 0xA6: // case 0x65:
                        return(4);
		case 0x66: // case 0x66:
                        return(5);
		case 0x96: // case 0x69:
                        return(6);
		case 0x56: // case 0x6A:
                        return(7);
		case 0xA9: // case 0x95:
                        return(8);
		case 0x69: // case 0x96:
                        return(9);
                default:
                        return(-1);
        }
}


/*
 * Examples of expected lines:
 * serialDecode: score (l=58), 8F A5 6A AA AA AA AA AA 9A AA A6 AA AA AA AA AA AA 6A AA A6 AA AA AA AA AA AA AA AA AA AA 55 AA 55 AA 55 55 55 AA 6A AA 55 6A AA 55 AA 55 55 55 55 55 55 55 55 A9 69 6A 55 F1
 */

void scoreBoardDecode(ScoreBoard *sb){
	// fprintf(stderr, "%s(%s)" "\n", __func__, sb->data);
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
		// fprintf(stderr, "no coma found" "\n");
		return;
	}

#if 0
	for(int i = 0 ; i < sizeof(binaries) ; i++){
		// fprintf(stderr, "[%2d]=%02X ", i, binaries[i]);
	}
	fputc('\n', stderr);
#endif
		if((0 == memcmp(binaries, "\x8F\xA5", 2)) && (0xF1 == binaries[sizeof(binaries) - 1])){
			// fprintf(stderr, "%s: correct sync and last byte" "\n", __func__);
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
				for(int i = 0 ; i < 4 ; i++){
					sb->timers[i] = nibblesToValue(binaries[44 - i - 12]);
				}

				int left_to_mask = nibblesToValue(binaries[26 - 12]); //  bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)
				int right_to_mask = nibblesToValue(binaries[25 - 12]); // bit 0 is first TO, bit 1 second TO (used during TO countdown, probably a blink mask)

				int checkScore(int values[4]){
					for(int i = 0 ; i < 4 ; i++){
						if(-1 == values[i]){
							return(-1);
						}
					}
					return 0;
				}

				if((0 <= setValue) && (setValue < 5)){
					// fprintf(stderr, "No Error Detected" "\n");
					sb->currentSet = setValue;
					// int localScore = 10 * localScoreMSB + localScoreLSB;
					// int visitorScore = 10 * visitorScoreMSB + visitorScoreLSB;
					sb->local.setScoreDigits[setValue][0] = localScoreMSB;
					sb->local.setScoreDigits[setValue][1] = localScoreLSB;
					sb->visitor.setScoreDigits[setValue][0] = visitorScoreMSB;
					sb->visitor.setScoreDigits[setValue][1] = visitorScoreLSB;
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
#if 0
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
#endif
					// In case we missed some point in the 2 first sets,
					// we can get them from set1Value and set2Value
					if(setValue > 0){
						if(0 == checkScore(set1Values)){
							sb->local.setScoreDigits[0][0] = set1Values[3];
							sb->local.setScoreDigits[0][1] = set1Values[2];
							sb->visitor.setScoreDigits[0][0] = set1Values[1];
							sb->visitor.setScoreDigits[0][1] = set1Values[0];
						}
					}
					if(setValue > 1){
						if(0 == checkScore(set2Values)){
							sb->local.setScoreDigits[1][0] = set2Values[3];
							sb->local.setScoreDigits[1][1] = set2Values[2];
							sb->visitor.setScoreDigits[1][0] = set2Values[1];
							sb->visitor.setScoreDigits[1][1] = set2Values[0];
						}
					}
					// fprintf(stderr, "Score:");
					for(int i = 0 ; i <= setValue ; i++){
						// fprintf(stderr, " %1d%1d/%1d%1d", sb->local.setScoreDigits[i][0], sb->local.setScoreDigits[i][1], sb->visitor.setScoreDigits[i][0], sb->visitor.setScoreDigits[i][1]);
					}
					// fprintf(stderr, "\n");
					// fprintf(stderr, "TO/TOM %x/%x [%d %d : %d %d] %x/%x TOM/TO" "\n", localTimeoutValue, left_to_mask, sb->timers[0], sb->timers[1], sb->timers[2], sb->timers[3], right_to_mask, visitorTimeoutValue);
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

/**
 * CRC-16 with the following parameters:
 *  - Width:          16 bits
 *  - Polynomial:    0x8005  (normal form: x^16 + x^15 + x^2 + 1)
 *  - Input reflected:   No  (MSb-first bit order)
 *  - Output reflected:  No  (MSb-first big endian)
 *  - Initial value:  0x0000
 *  - Final XOR:      0xFE28
 *
 * @param data   Pointer to the byte array
 * @param length Number of bytes in the array
 * @return       16-bit CRC
 */
uint16_t crc16_0x8005(const uint8_t *data, size_t length)
{
    uint16_t crc = 0x0000;          // Initial value

    for (size_t i = 0; i < length; ++i)
    {
        uint8_t byte = data[i];

        crc ^= (uint16_t)byte << 8;   // XOR byte into top of CRC

        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x8005;
            else
                crc <<= 1;
        }
    }

    // Apply final XOR
    crc ^= 0xFE28;

    return crc;
}

static int check_frame(ScoreBoard *sb){
	const char *coma = strchr(sb->data, ',');
	unsigned char binaries[58];
	memset(binaries, 0, sizeof(binaries));
	if(NULL != coma){
		const char *start = coma + 2;
		for(int i = 0 ; i < 58 ; i++){
			int hexa = 0;
			sscanf(start + 3 * i, "%02X", &hexa);
			binaries[i] = hexa;
		}
		if((0x8F == binaries[0]) && (0xF1 == binaries[sizeof(binaries) - 1])){
			uint16_t crc = crc16_0x8005(binaries, sizeof(binaries) - 3); // don't include CRC and terminating 0xF1
			const uint16_t expectedCRC = (binaries[55] << 8) | binaries[56];
			if(expectedCRC == crc){
				fprintf(stderr, "crc=0x%04X, OK !!!!" "\n", crc);
				return(1);
			}else{
				fprintf(stderr, "crc=0x%04X, expected 0x%04X" "\n", crc, expectedCRC);
			}
		}
	}
	return(0); // check failed
}

int scoreBoardAnalyze(ScoreBoard *sb, char *buffer, int n){
	int i = n;
	char *p = buffer;
	while(i--){
		char car = *p++;
		if('\n' == car){
			char *coma = strchr(sb->data, ',');
			if((NULL != coma) && check_frame(sb)){
				int comaOffset = coma - sb->data;
				// fprintf(stderr, "comaOffset=%d(%d)""[%s]" "\n", comaOffset, (sb->index - comaOffset) / 3, sb->data + comaOffset + 28);
				if(memcmp(oldScoreBoard.data + comaOffset + (9 * 3 + 1), sb->data + comaOffset + (9 * 3 + 1), sb->index - (comaOffset + (9 * 3 + 1) + (4 * 3 + 1)))){ // ignore first 9 and last 4 "bytes"
					// fprintf(stderr, "change detected => redrawing" "\n" "was: [%s]" "\n" "now: [%s]" "\n", oldScoreBoard.data, sb->data);
					scoreBoardDecode(sb);
					scoreBoardDraw(sb);
					scoreBoardJpegOutput(sb, stdout);
					strcpy(oldScoreBoard.data, sb->data);
				}
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

	// Unit test of crc function
	if(0){
		unsigned char input[55] = { 241, 165, 86, 85, 85, 85, 85, 85, 89, 85, 101, 85, 85, 85, 85, 85, 85, 86, 85, 101, 90, 85, 85, 85, 85, 85, 85, 85, 85, 85, 170, 85, 170, 86, 170, 170, 170, 86, 89, 89, 170, 101, 86, 170, 85, 170, 170, 170, 170, 102, 89, 101, 86, 102, 89 };
		uint16_t crc = crc16_0x8005(input, sizeof(input));
		fprintf(stderr, "crc=0x%04X" "\n", crc);
		exit(0);
		
	}
	// const char *fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
	const char *fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-BoldOblique.ttf";

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
		struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000};
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
				// fprintf(stderr, "Traffic timeout expired: %lds idle" "\n", delta_t);
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


