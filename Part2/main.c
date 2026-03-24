#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>

void apply_sobel_chunk(int *input, int *output, int width, int local_height, int total_height, int rank, int size) {
    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    // OpenMP parallelizes the row processing
    #pragma omp parallel for collapse(2)
    for (int y = 1; y < local_height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            float sumX = 0, sumY = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int pixel = input[(y + i) * width + (x + j)];
                    sumX += pixel * Gx[i + 1][j + 1];
                    sumY += pixel * Gy[i + 1][j + 1];
                }
            }
            int mag = (int)sqrt(sumX * sumX + sumY * sumY);
            output[y * width + x] = (mag > 255) ? 255 : mag;
        }
    }
}

int main(int argc, char** argv) {
    int width = 5000, height = 5000; 
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_height = height / size;
    int chunk_size = width * local_height;
    
    // We need 2 extra rows for the "Halo" (top and bottom neighbors)
    int *local_input = calloc(width * (local_height + 2), sizeof(int));
    int *local_output = calloc(width * (local_height + 2), sizeof(int));
    int *full_image = NULL;

    if (rank == 0) {
        full_image = malloc(width * height * sizeof(int));
        FILE *f = fopen("pixels.txt", "r");
        for (int i = 0; i < width * height; i++) fscanf(f, "%d", &full_image[i]);
        fclose(f);
    }

    // Distribute the image chunks to all processes
    MPI_Scatter(full_image, chunk_size, MPI_INT, 
                &local_input[width], chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    // --- Halo Exchange ---
    int top_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    int bot_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    // Send bottom row, receive into top halo
    MPI_Sendrecv(&local_input[chunk_size], width, MPI_INT, bot_neighbor, 0,
                 &local_input[0], width, MPI_INT, top_neighbor, 0, 
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Send top row, receive into bottom halo
    MPI_Sendrecv(&local_input[width], width, MPI_INT, top_neighbor, 1,
                 &local_input[chunk_size + width], width, MPI_INT, bot_neighbor, 1, 
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    apply_sobel_chunk(local_input, local_output, width, local_height + 2, height, rank, size);

    // Gather results back to Rank 0
    int *final_edges = NULL;
    if (rank == 0) final_edges = malloc(width * height * sizeof(int));

    MPI_Gather(&local_output[width], chunk_size, MPI_INT, 
               final_edges, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        FILE *f_out = fopen("output_edges.txt", "w");
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) fprintf(f_out, "%d ", final_edges[i * width + j]);
            fprintf(f_out, "\n");
        }
        fclose(f_out);
        free(full_image);
        free(final_edges);
    }

    free(local_input);
    free(local_output);
    MPI_Finalize();
    return 0;
}