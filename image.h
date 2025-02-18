#include <png.h>

typedef struct {
    int success;
    int num_colors;
    int bitplanes;
    int width;
    int height;
    png_colorp palette;
	unsigned char* palette_order;
    unsigned char *data;
} Image;

void free_image(Image *image);

Image read_png_indexed(char *input_file);

void write_png_indexed(const char *filename, const Image *image);

void c2p(const Image *image, unsigned char *bpl_data, int interleaved);
