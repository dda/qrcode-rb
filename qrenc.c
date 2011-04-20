// gcc -g -c qrenc.c -o quickQR.o -I/usr/local/include/libpng12
// gcc -dynamiclib -o quickQR.dylib quickQR.o -lz -lqrencode -L/usr/local/lib -lpng12


/**
 * qrencode - QR Code encoder
 *
 * QR Code encoding tool
 * Copyright (C) 2006, 2007, 2008, 2009 Kentaro Fukuchi <fukuchi@megaui.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <getopt.h>

#include "config.h"
#include "qrencode.h"

static int casesensitive = 1;
static int eightbit = 0;
static int version = 0;
static int size = 3;
static int margin = 4;
static int structured = 0;
static QRecLevel level = QR_ECLEVEL_L;
static QRencodeMode hint = QR_MODE_8;

static const struct option options[] = {
	{"help"         , no_argument      , NULL, 'h'},
	{"output"       , required_argument, NULL, 'o'},
	{"level"        , required_argument, NULL, 'l'},
	{"size"         , required_argument, NULL, 's'},
	{"symversion"   , required_argument, NULL, 'v'},
	{"margin"       , required_argument, NULL, 'm'},
	{"structured"   , no_argument      , NULL, 'S'},
	{"kanji"        , no_argument      , NULL, 'k'},
	{"casesensitive", no_argument      , NULL, 'c'},
	{"ignorecase"   , no_argument      , NULL, 'i'},
	{"8bit"         , no_argument      , NULL, '8'},
	{"version"      , no_argument      , NULL, 'V'},
	{NULL, 0, NULL, 0}
};

static char *optstring = "ho:l:s:v:m:Skci8V";


#define MAX_DATA_SIZE (7090 * 16) /* from the specification */
extern char *readStdin(void) {
	char *buffer;
	int ret;

	buffer = (char *)malloc(MAX_DATA_SIZE);
	if(buffer == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		exit(EXIT_FAILURE);
	}
	ret = fread(buffer, 1, MAX_DATA_SIZE, stdin);
	if(ret == 0) {
		fprintf(stderr, "No input data.\n");
		exit(EXIT_FAILURE);
	}
	if(feof(stdin) == 0) {
		fprintf(stderr, "Input data is too large.\n");
		exit(EXIT_FAILURE);
	}

	buffer[ret] = '\0';

	return buffer;
}

extern int writePNG(QRcode *qrcode, const char *outfile) {
	static FILE *fp; // avoid clobbering by setjmp.
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *row, *p, *q;
	int x, y, xx, yy, bit;
	int realwidth;

	realwidth = (qrcode->width + margin * 2) * size;
	row = (unsigned char *)malloc((realwidth + 7) / 8);
	if(row == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	if(outfile[0] == '-' && outfile[1] == '\0') {
		fp = stdout;
	} else {
		fp = fopen(outfile, "wb");
		if(fp == NULL) {
			fprintf(stderr, "Failed to create file: %s\n", outfile);
			perror(NULL);
			exit(EXIT_FAILURE);
		}
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG writer.\n");
		exit(EXIT_FAILURE);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG write.\n");
		exit(EXIT_FAILURE);
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fprintf(stderr, "Failed to write PNG image.\n");
		exit(EXIT_FAILURE);
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr,
			realwidth, realwidth,
			1,
			PNG_COLOR_TYPE_GRAY,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	/* top margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	/* data */
	p = qrcode->data;
	for(y=0; y<qrcode->width; y++) {
		bit = 7;
		memset(row, 0xff, (realwidth + 7) / 8);
		q = row;
		q += margin * size / 8;
		bit = 7 - (margin * size % 8);
		for(x=0; x<qrcode->width; x++) {
			for(xx=0; xx<size; xx++) {
				*q ^= (*p & 1) << bit;
				bit--;
				if(bit < 0) {
					q++;
					bit = 7;
				}
			}
			p++;
		}
		for(yy=0; yy<size; yy++) {
			png_write_row(png_ptr, row);
		}
	}
	/* bottom margin */
	memset(row, 0xff, (realwidth + 7) / 8);
	for(y=0; y<margin * size; y++) {
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	free(row);

	return 0;
}

extern QRcode *encode(const char *intext) {
	QRcode *code;
	if(eightbit) {
		printf("version: %i, level: %i, eightbit: %i\n", version, level, eightbit);
		code = QRcode_encodeString8bit(intext, version, level);
	} else {
		printf("version: %i, level: %i, hint: %i, case: %i, eightbit: %i\n", version, level, hint, casesensitive, eightbit);
		code = QRcode_encodeString(intext, version, level, hint, casesensitive);
	}

	return code;
}

extern void qrencode(const char *intext, const char *outfile) {
	QRcode *qrcode;
	
	qrcode = encode(intext);
	if(qrcode == NULL) {
		perror("Failed to encode the input data:");
		exit(EXIT_FAILURE);
	}
	writePNG(qrcode, outfile);
	QRcode_free(qrcode);
}

extern QRcode_List *encodeStructured(const char *intext) {
	QRcode_List *list;

	if(eightbit) {
		list = QRcode_encodeString8bitStructured(intext, version, level);
	} else {
		list = QRcode_encodeStringStructured(intext, version, level, hint, casesensitive);
	}

	return list;
}

extern void qrencodeStructured(const char *intext, const char *outfile) {
	QRcode_List *qrlist, *p;
	char filename[FILENAME_MAX];
	char *base, *q, *suffix = NULL;
	int i = 1;

	base = strdup(outfile);
	if(base == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	if(strlen(base) > 4) {
		q = base + strlen(base) - 4;
		if(strcasecmp(".png", q) == 0) {
			suffix = strdup(q);
			*q = '\0';
		}
	}
	
	qrlist = encodeStructured(intext);
	if(qrlist == NULL) {
		perror("Failed to encode the input data:");
		exit(EXIT_FAILURE);
	}

	for(p = qrlist; p != NULL; p = p->next) {
		if(p->code == NULL) {
			fprintf(stderr, "Failed to encode the input data.\n");
			exit(EXIT_FAILURE);
		}
		if(suffix) {
			snprintf(filename, FILENAME_MAX, "%s-%02d%s", base, i, suffix);
		} else {
			snprintf(filename, FILENAME_MAX, "%s-%02d", base, i);
		}
		writePNG(p->code, filename);
		i++;
	}

	free(base);
	if(suffix) {
		free(suffix);
	}

	QRcode_List_free(qrlist);
}

int main(int argc, char **argv) {
	int opt, lindex = -1;
	char *outfile = NULL;
	char *intext = NULL;

	while((opt = getopt_long(argc, argv, optstring, options, &lindex)) != -1) {
		switch(opt) {
			case 'h':
				if(lindex == 0) {
					usage(1, 1);
				} else {
					usage(1, 0);
				}
				exit(0);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 's':
				size = atoi(optarg);
				if(size <= 0) {
					fprintf(stderr, "Invalid size: %d\n", size);
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				version = atoi(optarg);
				if(version < 0) {
					fprintf(stderr, "Invalid version: %d\n", version);
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				switch(*optarg) {
					case 'l':
					case 'L':
						level = QR_ECLEVEL_L;
						break;
					case 'm':
					case 'M':
						level = QR_ECLEVEL_M;
						break;
					case 'q':
					case 'Q':
						level = QR_ECLEVEL_Q;
						break;
					case 'h':
					case 'H':
						level = QR_ECLEVEL_H;
						break;
					default:
						fprintf(stderr, "Invalid level: %s\n", optarg);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case 'm':
				margin = atoi(optarg);
				if(margin < 0) {
					fprintf(stderr, "Invalid margin: %d\n", margin);
					exit(EXIT_FAILURE);
				}
				break;
			case 'S':
				structured = 1;
			case 'k':
				hint = QR_MODE_KANJI;
				break;
			case 'c':
				casesensitive = 1;
				break;
			case 'i':
				casesensitive = 0;
				break;
			case '8':
				eightbit = 1;
				break;
			case 'V':
				usage(0, 0);
				exit(0);
				break;
			default:
				fprintf(stderr, "Try `qrencode --help' for more information.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(argc == 1) {
		usage(1, 0);
		exit(0);
	}

	if(outfile == NULL) {
		fprintf(stderr, "No output filename is given.\n");
		exit(EXIT_FAILURE);
	}

	if(optind < argc) {
		intext = argv[optind];
	}
	if(intext == NULL) {
		intext = readStdin();
	}

	if(structured) {
		if(version == 0) {
			fprintf(stderr, "Version must be specified to encode structured symbols.\n");
			exit(EXIT_FAILURE);
		}
		qrencodeStructured(intext, outfile);
	} else {
		qrencode(intext, outfile);
	}

	return 0;
}
