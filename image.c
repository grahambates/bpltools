// Shared logic for reading / writing / converting PNG data

#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "image.h"
#include "log.h"
#include "safe_mem.h"

void free_image(Image *image) {
    if (image->palette) {
		free(image->palette);
		image->palette = 0;
	}
    if (image->data) {
		free(image->data);
		image->data = 0;
	}
}

// Load source image
Image read_png_indexed(char *input_file) {
	verbose_log("Reading PNG: %s\n", input_file);
    Image image = {0};
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        error_log("Failed to open file %s\n", input_file);
        return image;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        error_log("Error: Failed to create PNG read struct.\n");
        return image;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        error_log("Error: Failed to create PNG info struct.\n");
        png_destroy_read_struct(&png, NULL, NULL);
        return image;
    }

    if (setjmp(png_jmpbuf(png))) {
        error_log("Error: PNG reading failed.\n");
        png_destroy_read_struct(&png, &info, NULL);
        return image;
    }

    png_init_io(png, fp);
    png_read_info(png, info);
    if (png_get_color_type(png, info) != PNG_COLOR_TYPE_PALETTE || png_get_bit_depth(png, info) != 8) {
        error_log("Error: Not an 8-bit indexed PNG.\n");
        png_destroy_read_struct(&png, &info, NULL);
        return image;
    }

    png_colorp temp_palette;
    int num_colors;
    if (!png_get_PLTE(png, info, &temp_palette, &num_colors)) {
        error_log("Error: Failed to get PNG palette.\n");
        png_destroy_read_struct(&png, &info, NULL);
        return image;
    }

    image.palette = safe_malloc(num_colors * sizeof(png_color));

    memcpy(image.palette, temp_palette, num_colors * sizeof(png_color));
    image.palette_order = safe_malloc(num_colors);
	for (int i = 0; i < num_colors; i++) {
		image.palette_order[i] = i;
	}

    image.num_colors = num_colors;
    image.width = png_get_image_width(png, info);
    image.height = png_get_image_height(png, info);
    image.bitplanes = (int)ceil(log2(image.num_colors));

    if (image.width % 16) {
        error_log("Error: Image width must be a multiple of 16.\n");
        free(image.palette);
        png_destroy_read_struct(&png, &info, NULL);
        return image;
    }

    image.data = safe_malloc(image.width * image.height);

    png_bytep *row_pointers = safe_malloc(sizeof(png_bytep) * image.height);

    for (int y = 0; y < image.height; y++) {
        row_pointers[y] = &image.data[y * image.width];
    }

    png_read_image(png, row_pointers);
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);

    image.success = 1;
    return image;
}

// Write image with reordered palette
void write_png_indexed(const char *filename, const Image *image) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        error_log("Error: Could not open %s for writing.\n", filename);
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        error_log("Error: Failed to create PNG write struct.\n");
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        error_log("Error: Failed to create PNG info struct.\n");
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        error_log("Error: PNG writing failed.\n");
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    // Set PNG header info
    png_set_IHDR(png, info, image->width, image->height, 8, PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Update the palette order in the PNG
    png_color *new_palette = safe_malloc(image->num_colors * sizeof(png_color));
    for (int i = 0; i < image->num_colors; i++) {
        new_palette[image->palette_order[i]] = image->palette[i];
    }
    png_set_PLTE(png, info, new_palette, image->num_colors);
    free(new_palette);

    png_write_info(png, info);

    // Remap pixel values based on the new palette order
    unsigned char *remapped_data = safe_malloc(image->width * image->height);
    for (int i = 0; i < image->width * image->height; i++) {
        remapped_data[i] = image->palette_order[image->data[i]];
    }

    png_bytep *row_pointers = safe_malloc(sizeof(png_bytep) * image->height);
    for (int y = 0; y < image->height; y++) {
        row_pointers[y] = &remapped_data[y * image->width];
    }

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);
    free(remapped_data);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    printf("Updated PNG written to %s\n", filename);
}

// Chunky to planar conversion
void c2p(const Image *image, unsigned char *bpl_data, int interleaved) {
    int byte_width = image->width / 8;
    int row_size = interleaved ? image->bitplanes * byte_width : byte_width;
    int bpl_offset = interleaved ? byte_width : image->height * byte_width;

    memset(bpl_data, 0, byte_width * image->height * image->bitplanes);

	int i = 0;
    for (int y = 0; y < image->height; y++) {
        int row_start = y * row_size;

        for (int x = 0; x < byte_width; x++) {
            unsigned char plane_bytes[8] = {0};

            for (int p = 0; p < 8; p++) {
                unsigned char mapped_idx = image->palette_order[image->data[i++]];
                for (int bpl = 0; bpl < image->bitplanes; bpl++) {
                    if (mapped_idx & (1 << bpl)) {
                        plane_bytes[bpl] |= (1 << (7 - p));
                    }
                }
            }

            for (int bpl = 0; bpl < image->bitplanes; bpl++) {
                bpl_data[row_start + x + bpl_offset * bpl] = plane_bytes[bpl];
            }
        }
    }
}
