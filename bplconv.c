// Converts 8 bit indexed PNGs to bitplane data

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "image.h"
#include "log.h"

uint16_t convert12bit(png_color col) {
	unsigned int r = col.red >> 4;   // Convert 8-bit to 4-bit
	unsigned int g = col.green >> 4;
	unsigned int b = col.blue >> 4;
	return (r << 8) | (g << 4) | b;  // Amiga 12-bit RGB format
}

uint16_t swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

void export_palette_raw(const Image *image, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        error_log("Error: Could not open %s for writing.\n", filename);
        return;
    }
    uint16_t *palette = malloc(image->num_colors * 2);
    for (int i = 0; i < image->num_colors; i++) {
		unsigned char k = image->palette_order[i];
        palette[k] = swap16(convert12bit(image->palette[i]));
    }
    fwrite(palette, 2, image->num_colors, fp);
    fclose(fp);
}

void export_palette_copper(const Image *image, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        error_log("Error: Could not open %s for writing.\n", filename);
        return;
    }
    uint16_t *palette = malloc(image->num_colors * 4);
    for (int i = 0; i < image->num_colors; i++) {
		unsigned char k = image->palette_order[i];
        palette[k*2] = swap16(0x180 + k * 2);
        palette[k*2+1] = swap16(convert12bit(image->palette[i]));
    }
    fwrite(palette, 4, image->num_colors, fp);
    fclose(fp);
}

void export_bitplane_data(const Image *image, const char *output_file, int interleaved) {
    int bpl_size = (image->width / 8) * image->height * image->bitplanes;
    unsigned char *bpl_data = malloc(bpl_size);
    if (!bpl_data) {
        error_log("Error: Memory allocation failed for bitplane data.\n");
        return;
    }

	verbose_log("Interleaved mode: %s\n", interleaved ? "ON" : "OFF");
    c2p(image, bpl_data, interleaved);

    FILE *fpout = fopen(output_file, "wb");
    if (!fpout) {
        error_log("Could not open file %s for writing\n", output_file);
        free(bpl_data);
        return;
    }

    fwrite(bpl_data, 1, bpl_size, fpout);
    fclose(fpout);
    free(bpl_data);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options] <image.png> <output_file>\n", prog_name);
    printf("Options:\n");
    printf("  -i, --interleaved          Enable interleaved mode\n");
    printf("  -r, --raw-palette=FILE     Export raw palette\n");
    printf("  -c, --copper-palette=FILE  Export palette as copper list\n");
    printf("  -v, --verbose              Enable verbose output\n");
    printf("  -h, --help                 Display this help message\n");
}

int main(int argc, char *argv[]) {
    int interleaved = 0;
    char *raw_palette_file = NULL;
    char *copper_palette_file = NULL;
    int opt;

    // Define long options
    static struct option long_options[] = {
        {"interleaved", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {"raw-palette", required_argument, 0, 'r'},
        {"copper-palette", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "ivr:c:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i':
                interleaved = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'r':
                raw_palette_file = optarg;
                break;
            case 'c':
                copper_palette_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                error_log("Invalid option. Use -h for help.\n");
                return EXIT_FAILURE;
        }
    }

    // Ensure we have at least two positional arguments: input and output file
    if (optind + 2 != argc) {
        error_log("Error: Incorrect number of arguments.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *input_file = argv[optind];
    char *output_file = argv[optind + 1];

    Image image = read_png_indexed(input_file);
    if (!image.success) {
        error_log("Error reading PNG data\n");
        return EXIT_FAILURE;
    }
	verbose_log("%d x %d, %d colors\n", image.width, image.height, image.num_colors);

    // Export Palette if requested
    if (raw_palette_file) {
		verbose_log("Raw palette export: %s\n", raw_palette_file);
        export_palette_raw(&image, raw_palette_file);
    }
    if (copper_palette_file) {
		verbose_log("Copper palette export: %s\n", copper_palette_file);
        export_palette_copper(&image, copper_palette_file);
    }

	// Export bitplane data
	verbose_log("Bitplane data export: %s\n", output_file);
	export_bitplane_data(&image, output_file, interleaved);
	free_image(&image);

	verbose_log("Conversion complete!\n");
    return EXIT_SUCCESS;
}
