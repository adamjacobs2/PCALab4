#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void apply_sobel(int *input, int *output, int width, int height) {
    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            float sumX = 0, sumY = 0;

            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int pixel = input[(y + i) * width + (x + j)];
                    sumX += pixel * Gx[i + 1][j + 1];
                    sumY += pixel * Gy[i + 1][j + 1];
                }
            }
        
            int magnitude = (int)sqrt(sumX * sumX + sumY * sumY);
            output[y * width + x] = (magnitude > 255) ? 255 : magnitude;
        }
    }
}
int main() {
    int width = 5000, height = 5000; 
    int *pixels = malloc(width * height * sizeof(int));
    int *edges = calloc(width * height, sizeof(int));

    FILE *file = fopen("processed_matrix.txt", "r");
    if (!file || !pixels) return 1;

    for (int i = 0; i < width * height; i++) {
        if (fscanf(file, "%d", &pixels[i]) != 1) break;
    }

    apply_sobel(pixels, edges, width, height);

    FILE *f_out = fopen("output_edges.txt", "w");
    if (!f_out) {
        perror("Error creating output file");
        return 1;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fprintf(f_out, "%d ", edges[y * width + x]);
        }
        fprintf(f_out, "\n");
    }
    fclose(f_out);


    fclose(file);
    free(pixels);
    free(edges);
    return 0;
}