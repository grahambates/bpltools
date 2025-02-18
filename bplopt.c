// Reorders palette of an indexed PNG for optimal LZ compression size of converted bitplane data

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <zlib.h>

#include "image.h"
#include "log.h"
#include "safe_mem.h"

// Simulated-annealing settings
#define INITIAL_TEMPERATURE 1000.0   // Starting temperature
#define COOLING_RATE 0.99            // Cooling multiplier (0.99 means slow cooling)
#define MIN_TEMPERATURE 0.1          // Stop when temperature is very low
#define ITERATIONS_PER_TEMP 20       // Number of swaps per temperature step

static int *locked_map = NULL;

void parse_locked_indexes(char *arg, int num_colors) {
    char *token = strtok(arg, ",");
    locked_map = safe_calloc(num_colors, sizeof(int));

    while (token) {
        int index = atoi(token);
        if (index >= 0 && index < num_colors) {
            locked_map[index] = 1;
        } else {
            error_log("Warning: Ignoring out-of-bounds lock index %d\n", index);
        }
        token = strtok(NULL, ",");
    }
}

static inline int is_locked(int index) {
    return locked_map && locked_map[index];
}

// Greedy hill climbing algorithm with non-adjacent swaps
void find_optimal_palette(Image *image, unsigned char *bpl_data, int bpl_size, int interleaved) {
    uLongf compressed_size = compressBound(bpl_size);
    unsigned char *compressed_data = (unsigned char *)safe_malloc(compressed_size);

    // Get initial compressed size
    c2p(image, bpl_data, interleaved);
    compress(compressed_data, &compressed_size, bpl_data, bpl_size);
    uLongf best_size = compressed_size;
    printf("%lu\n", best_size);

    unsigned char tmp;
    int improved = 1;

    while (improved) {
        improved = 0;
        for (int i = 0; i < image->num_colors; i++) {
            if (is_locked(i)) continue;
            for (int j = i + 1; j < image->num_colors; j++) {
                if (is_locked(j)) continue;
                // Swap pair
                tmp = image->palette_order[i];
                image->palette_order[i] = image->palette_order[j];
                image->palette_order[j] = tmp;

                // Convert and compress new palette order
                c2p(image, bpl_data, interleaved);
                compress(compressed_data, &compressed_size, bpl_data, bpl_size);

                // New best size?
                if (compressed_size < best_size) {
                    // keep change
                    improved = 1;
                    best_size = compressed_size; // keep change
                    printf("%lu\n", best_size);
                } else {
                    // swap back
                    tmp = image->palette_order[i];
                    image->palette_order[i] = image->palette_order[j];
                    image->palette_order[j] = tmp;
                }
            }
        }
    }

    free(compressed_data);
}

// Simulated-annealing
void find_optimal_palette_sa(Image *image, unsigned char *bpl_data, int bpl_size, int interleaved) {
    uLongf compressed_size = compressBound(bpl_size);
    unsigned char *compressed_data = (unsigned char *)safe_malloc(compressed_size);

    // Get initial compressed size
    c2p(image, bpl_data, interleaved);
    compress(compressed_data, &compressed_size, bpl_data, bpl_size);
    uLongf best_size = compressed_size;
    printf("%lu\n", best_size);

    // Copy initial order
    unsigned char *best_order = (unsigned char *)safe_malloc(image->num_colors);
    memcpy(best_order, image->palette_order, image->num_colors);

    double T = INITIAL_TEMPERATURE;

    while (T > MIN_TEMPERATURE) {
        for (int iter = 0; iter < ITERATIONS_PER_TEMP; iter++) {
            // Pick two random indices to swap
            int i, j;
            do {
                i = rand() % image->num_colors;
            } while (is_locked(i));

            do {
                j = rand() % image->num_colors;
            } while (is_locked(j) || j == i);

            // Swap colors
            unsigned char tmp = image->palette_order[i];
            image->palette_order[i] = image->palette_order[j];
            image->palette_order[j] = tmp;

            // Recompute compressed size
            c2p(image, bpl_data, interleaved);
            uLongf new_size = compressBound(bpl_size);
            compress(compressed_data, &new_size, bpl_data, bpl_size);

            // Compute acceptance probability
            double delta = (double)(new_size - compressed_size);
            double probability = exp(-delta / T);

            // Accept the new order if it's better, or with probability `e^(-ΔE/T)`
            if (new_size < compressed_size || ((double)rand() / RAND_MAX) < probability) {
                compressed_size = new_size;
                if (new_size < best_size) {
                    best_size = new_size;
                    memcpy(best_order, image->palette_order, image->num_colors);
                    printf("%lu\n", best_size);
                }
            } else {
                // Revert swap if not accepted
                tmp = image->palette_order[i];
                image->palette_order[i] = image->palette_order[j];
                image->palette_order[j] = tmp;
            }
        }

        // Cool down
        T *= COOLING_RATE;
    }

    // Restore the best palette order found
    memcpy(image->palette_order, best_order, image->num_colors);

    free(compressed_data);
    free(best_order);
}

void print_palette(const Image *image) {
    // Need to invert order mappings
    uint16_t *palette = safe_malloc(image->num_colors);
    for (int i = 0; i < image->num_colors; i++) {
        palette[image->palette_order[i]] = i;
    }
    printf("Palette order:\n%d", palette[0]);
    for (int i = 1; i < image->num_colors; i++) {
        printf(", %d", palette[i]);
    }
    printf("\n");
    free(palette);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options] <input.png> <output.png>\n", prog_name);
    printf("Options:\n");
    printf("  -i, --interleaved          Enable interleaved mode\n");
    printf("  -s, --simulated-annealing  Use simulated annealing\n");
    printf("  -l, --lock=INDEXES         Lock palette indexes (comma separated)\n");
    printf("  -v, --verbose              Enable verbose output\n");
    printf("  -h, --help                 Display this help message\n");
}

int main(int argc, char *argv[]) {
    int interleaved = 0;
    int sa = 0;
    char *lock_list = NULL;
    int opt;

    // Define long options
    static struct option long_options[] = {
        {"interleaved", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {"simulated-annealing", no_argument, 0, 's'},
        {"lock", required_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "ivsr:c:b:l:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i': interleaved = 1; break;
            case 'v': verbose = 1; break;
            case 's': sa = 1; break;
            case 'l': lock_list = optarg; break;
            case 'h': print_usage(argv[0]); return EXIT_SUCCESS;
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

    // Allocate bitplane data
    int bpl_size = (image.width / 8) * image.height * image.bitplanes;
    unsigned char *bpl_data = safe_malloc(bpl_size);

    if (lock_list) {
        parse_locked_indexes(lock_list, image.num_colors);
    }

    verbose_log("Interleaved mode: %s\n", interleaved ? "ON" : "OFF");

    if (sa) {
        verbose_log("Using simulated annealing algorithm\n");
        find_optimal_palette_sa(&image, bpl_data, bpl_size, interleaved);
    } else {
        verbose_log("Using greedy hill climbing algorithm\n");
        find_optimal_palette(&image, bpl_data, bpl_size, interleaved);
    }
    free(bpl_data);

    print_palette(&image);

    // Save reordered png
    write_png_indexed(output_file, &image);

    free_image(&image);
    if (locked_map) free(locked_map);

    verbose_log("Optimisation complete!\n");
    return EXIT_SUCCESS;
}
