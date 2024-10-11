#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma pack(1)
typedef struct {
    char format_id[2];
    uint32_t file_size;
    uint16_t reserved_val1;
    uint16_t reserved_val2;
    uint32_t offset;
} BMP_Header;

typedef struct {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t color_planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_resolution;
    int32_t y_resolution;
    uint32_t colors_used;
    uint32_t important_colors;
} DIB_Header;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Pixel;
#pragma pack(4)

//function to calculate row padding (so each row is a multiple of 4 bytes)
int calculate_padding(int width) {
    int row_size = width * 3;
    return (4 - (row_size % 4)) % 4;
}


int read_headers(FILE *file, BMP_Header *bmpHeader, DIB_Header *dibHeader) {
    //read BMP header
    fread(bmpHeader, sizeof(BMP_Header), 1, file);

    //check for 'BM'
    if(bmpHeader->format_id[0] != 'B' || bmpHeader->format_id[1] != 'M') {
        printf("ERROR: The format is not supported.\n");
        return 1;
    }

    //read DIB header
    fread(dibHeader, sizeof(DIB_Header), 1, file);

    //check is DIB size = 40 and if bpp = 24
    if(dibHeader->header_size != 40 ||dibHeader->bits_per_pixel != 24 ) {
        printf("ERROR: The format is not supported.\n");
        return 1;
    }

    return 0;
}

void display_info(FILE *file, BMP_Header *bmpHeader, DIB_Header *dibHeader) {
    //print the bmp header info
    printf("=== BMP Header ===\n");
    printf("Type: %c%c\n", bmpHeader->format_id[0], bmpHeader->format_id[1]);
    printf("Size: %u\n", bmpHeader->file_size);
    printf("Reserved 1: %u\n", bmpHeader->reserved_val1);
    printf("Reserved 2: %u\n", bmpHeader->reserved_val2);
    printf("Image offset: %u\n", bmpHeader->offset);

    //print the bid header info
    printf("\n=== DIB Header ===\n");
    printf("Size: %u\n", dibHeader->header_size);
    printf("Width: %d\n", dibHeader->width);
    printf("Height: %d\n", dibHeader->height);
    printf("# color planes: %u\n", dibHeader->color_planes);
    printf("# bits per pixel: %u\n", dibHeader->bits_per_pixel);
    printf("Compression scheme: %u\n", dibHeader->compression);
    printf("Image size: %u\n", dibHeader->image_size);
    printf("Horizontal resolution: %d\n", dibHeader->x_resolution);
    printf("Vertical resolution: %d\n", dibHeader->y_resolution);
    printf("# colors in palette: %u\n", dibHeader->colors_used);
    printf("# important colors: %u\n", dibHeader->important_colors);
}

//swap 4 MSB and LSB for reveal
void reveal_image(FILE *file, BMP_Header *bmpHeader, DIB_Header *dibHeader) {
    //moves to start to pixel array
    fseek(file, bmpHeader->offset, SEEK_SET);

    int padding = calculate_padding(dibHeader->width);
    Pixel pixel;

    for(int y=0; y < dibHeader->height; y++) {
        for(int x=0; x <dibHeader->width; x++) {
            fread(&pixel, sizeof(Pixel), 1, file);

            //swap 4 MSB and LSB of each color channel
            pixel.blue = ((pixel.blue & 0xF0) >> 4) | ((pixel.blue & 0x0F) << 4);
            pixel.green = ((pixel.green & 0xF0) >> 4) | ((pixel.green & 0x0F) << 4);
            pixel.red = ((pixel.red & 0xF0) >> 4) | ((pixel.red & 0x0F) << 4);

            fseek(file, -(int)sizeof(Pixel), SEEK_CUR);
            fwrite(&pixel, sizeof(Pixel), 1, file);
        }

        //skip the padding bytes if necessary
        fseek(file, padding, SEEK_CUR); 
    }
}

//embed filename1 into filename2
void hide_image(FILE *file1, FILE *file2, BMP_Header *bmpHeader1, BMP_Header *bmpHeader2, DIB_Header *dibHeader1, DIB_Header *dibHeader2){
    fseek(file1, bmpHeader1->offset, SEEK_SET);
    fseek(file2, bmpHeader2->offset, SEEK_SET);

    int padding1 = calculate_padding(dibHeader1->width);
    int padding2 = calculate_padding(dibHeader2->width);

    Pixel pixel1;
    Pixel pixel2;

    for (int y = 0; y < dibHeader1->height; y++) {
        for (int x = 0; x < dibHeader1->width; x++) {
            fread(&pixel1, sizeof(Pixel), 1, file1);
            fread(&pixel2, sizeof(Pixel), 1, file2);

            /* debug print before mod
            printf("Before modification - Pixel1 (B: %u, G: %u, R: %u)\n", pixel1.blue, pixel1.green, pixel1.red);
            printf("Before modification - Pixel2 (B: %u, G: %u, R: %u)\n", pixel2.blue, pixel2.green, pixel2.red);
            */

            //replace 4 LSB of filename1 with 4 MSB of filename2
            pixel1.blue = (pixel1.blue & 0xF0) | ((pixel2.blue & 0xF0) >> 4);
            pixel1.green = (pixel1.green & 0xF0) | ((pixel2.green & 0xF0) >> 4);
            pixel1.red = (pixel1.red & 0xF0) | ((pixel2.red & 0xF0) >> 4);

            /* debug print post mod
            printf("After modification - Pixel1 (B: %u, G: %u, R: %u)\n", pixel1.blue, pixel1.green, pixel1.red);
            */

            fseek(file1, -(int)sizeof(Pixel), SEEK_CUR);
            fwrite(&pixel1, sizeof(Pixel), 1, file1);
        }

        //skips padding
        fseek(file1, padding1, SEEK_CUR);
        fseek(file2, padding2, SEEK_CUR);
    }
}

void process_pixel_data(FILE *file, BMP_Header *bmpHeader, DIB_Header *dibHeader) {
    //move file pointer to start of the pixel array using the offset value
    fseek(file, bmpHeader->offset, SEEK_SET);

    //now at beginning of pixel data, process it 
    Pixel pixel;

    //example of reading a single pixel
    fread(&pixel, sizeof(Pixel), 1, file);

    //print pixel values
    printf("First pixel - R: %u, G: %u, B: %u\n", pixel.red, pixel.green, pixel.blue);

    //could loop over for the whole image using width and height
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("ERROR: Missing arguments.\n");
        return 1;
    }

    FILE *file1 = fopen(argv[2], "r+b");  //open FILENAME1 in read/write mode
    
    if (!file1) {
        printf("ERROR: File not found: %s\n", argv[2]);
        return 1;
    }

    BMP_Header bmpHeader1, bmpHeader2;
    DIB_Header dibHeader1, dibHeader2;

    if (strcmp(argv[1], "--info") == 0) {
        //read headers and validate file
        if (read_headers(file1, &bmpHeader1, &dibHeader1)) {
            fclose(file1);
            return 1;
        }
        display_info(file1, &bmpHeader1, &dibHeader1);

    } else if (strcmp(argv[1], "--reveal") == 0) {
        //read headers before processing pixels
        if (read_headers(file1, &bmpHeader1, &dibHeader1)) {
            fclose(file1);
            return 1;
        }
        reveal_image(file1, &bmpHeader1, &dibHeader1);

    } else if (strcmp(argv[1], "--hide") == 0 && argc == 4) {
        FILE *file2 = fopen(argv[3], "rb"); //read only mode
        if(!file2){
            printf("ERROR: File not found: %s\n", argv[3]);
            fclose(file1);
            return 1;
        }

        //read headers for both files
        if(read_headers(file1, &bmpHeader1, &dibHeader1) || read_headers(file2, &bmpHeader2, &dibHeader2)) {
            fclose(file1);
            fclose(file2);
            return 1;
        }

        //check if they have the same dimensions
        if(dibHeader1.width != dibHeader2.width || dibHeader1.height != dibHeader2.height) {
            printf("ERROR: The images do not have the same dimensions.\n");
            fclose(file1);
            fclose(file2);
            return 1;
        }

        //hide filename2 in filename1
        hide_image(file1, file2, &bmpHeader1, &bmpHeader2, &dibHeader1, &dibHeader2);
        fclose(file2);

    } else {
        printf("ERROR: Missing arguments.\n");
        fclose(file1);
        return 1;
    }

    //close
    fclose(file1);

    return 0;
}