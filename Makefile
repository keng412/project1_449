CC=gcc
CFLAGS=-Wall -Werror

all: bmp_steganography

bmp_steganography: bmp_steganography.c
	$(CC) $(CFLAGS) -o bmp_steganography bmp_steganography.c
