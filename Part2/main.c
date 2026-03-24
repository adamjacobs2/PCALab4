/*mat_mult.c */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

/*   ttype: type to use for representing time */
typedef double ttype;
ttype tdiff(struct timespec a, struct timespec b)
/* Find the time difference. */
{
  ttype dt = (( b.tv_sec - a.tv_sec ) + ( b.tv_nsec - a.tv_nsec ) / 1E9);
  return dt;
}

struct timespec now()
/* Return the current time. */
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t;
}


void apply_sobel_omp(int *input, int *output, int width, int height) {
    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    #pragma omp parallel for collapse(2)
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

#define MASTER 0               /* taskid of first task */
#define FROM_MASTER 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */

int main (int argc, char *argv[]){


    int	numtasks,            /* number of tasks in partition */
    taskid,                /* a task identifier */
    numworkers,            /* number of worker tasks */
    source,                /* task id of message source */
    dest,                  /* task id of message destination */
    mtype,                 /* message type */
    rows,                  /* rows of matrix A sent to each worker */
    averow, extra, offset, /* used to determine rows sent to each worker */
    i, j, k, rc;           /* misc */

  
    MPI_Status status;

    //clock_t begin, end;
    struct timespec begin, end;
    double time_spent;
 
    

    
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);


    if (numtasks < 2 ){
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }
    numworkers = numtasks-1;
    int width = 5000, height = 5000; 

  /**************************** master task ************************************/
    if (taskid == MASTER){
        printf("mpi_mm has started with %d tasks.\n",numtasks);

    
        int *pixels = malloc(width * height * sizeof(int));
        int *edges = calloc(width * height, sizeof(int));  

        FILE *file = fopen("input.txt", "r");
        if (!file || !pixels) return 1;

        printf("Reading image...\n");
    
        for (int i = 0; i < width * height; i++) {
            if (fscanf(file, "%d", &pixels[i]) != 1) break;
        }
    
        /* Send matrix data to the worker tasks */
        averow = height/numworkers;
        extra = height%numworkers;
        offset = 0;
        mtype = FROM_MASTER;

        //begin = clock();
        begin = now();
        
        for (dest=1; dest<=numworkers; dest++) {
            rows = (dest <= extra) ? averow+1 : averow;

            // Logic for halo boundaries
            int start_row = (offset == 0) ? offset : offset - 1;
            int send_rows = rows;
            if (offset > 0) send_rows++;                 // Top halo
            if (offset + rows < height) send_rows++;     // Bottom halo

            MPI_Send(&offset, 1, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD);
            MPI_Send(&send_rows, 1, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD);

            MPI_Send(&pixels[start_row * width], send_rows * width, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD);

            offset = offset + rows; 
        }

        /* Receive results from worker tasks */
        mtype = FROM_WORKER;
        for (i=1; i<=numworkers; i++)
        {
          source = i;
          MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
          MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
          MPI_Recv(&edges[offset * width], rows*width, MPI_INT, source, mtype, 
                    MPI_COMM_WORLD, &status);
          printf("Received results from task %d\n",source);
        }

  
        end = now();
        time_spent = tdiff(begin, end);



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
          
        
        printf("\n******************************************************\n");
        printf ("\n");
        printf("total time: %.8f sec\n", time_spent);
        printf ("\n");

        free(pixels);
        free(edges);
    }



   /**************************** worker task ************************************/
    if (taskid > MASTER) {
        mtype = FROM_MASTER;
        int local_rows, original_offset;

        // Receive metadata from Master
        MPI_Recv(&original_offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
        MPI_Recv(&local_rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);

        int *workerPixels = malloc(width * local_rows * sizeof(int));
        int *workerEdges = calloc(width * local_rows, sizeof(int)); 

   
        MPI_Recv(workerPixels, local_rows * width, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);


        apply_sobel_omp(workerPixels, workerEdges, width, local_rows);


        int averow = height / numworkers;
        int extra = height % numworkers;
        int actual_rows = (taskid <= extra) ? averow + 1 : averow;
        
     
        int start_index = (original_offset == 0) ? 0 : 1;

        mtype = FROM_WORKER;
        MPI_Send(&original_offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
        MPI_Send(&actual_rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
        
        // Send back ONLY the clean rows (actual_rows), skipping the top halo row
        MPI_Send(&workerEdges[start_index * width], actual_rows * width, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);

        free(workerEdges);
        free(workerPixels);
    }



    MPI_Finalize();
   
}