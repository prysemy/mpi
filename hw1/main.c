#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int mode;

    if (strcmp(argv[1], "send") == 0)
        mode = 0;
    else if (strcmp(argv[1], "ssend") == 0)
        mode = 1;
    else
        mode = 2;

    int max_size = 1024 * 1024;

    char *buffer = malloc(max_size);

    int bsend_size = 2 * max_size + MPI_BSEND_OVERHEAD;
    void *bsend_buffer = malloc(bsend_size);

    if (mode == 2) {
        MPI_Buffer_attach(bsend_buffer, bsend_size);
    }

    FILE *file = NULL;

    if (rank == 0) {
        file = fopen(argv[2], "w");
        fprintf(file, "bytes,time_us,repetitions\n");
    }

    for (int n = 1; n <= max_size; n *= 2) {

        int repetitions = 8 * 1024 * 1024 / n;
	if (repetitions > 5000) repetitions = 5000;
	if (repetitions < 50) repetitions = 50;

        MPI_Barrier(MPI_COMM_WORLD);

        double start = 0;

        for (int i = -20; i < repetitions; i++) {

            if (i == 0) {
                MPI_Barrier(MPI_COMM_WORLD);

                if (rank == 0)
                    start = MPI_Wtime();
            }

            if (rank == 0) {

                if (mode == 0)
                    MPI_Send(buffer, n, MPI_BYTE,
                             1, 0, MPI_COMM_WORLD);

                else if (mode == 1)
                    MPI_Ssend(buffer, n, MPI_BYTE,
                              1, 0, MPI_COMM_WORLD);

                else
                    MPI_Bsend(buffer, n, MPI_BYTE,
                              1, 0, MPI_COMM_WORLD);

                MPI_Recv(buffer, n, MPI_BYTE,
                         1, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            }

            else {

                MPI_Recv(buffer, n, MPI_BYTE,
                         0, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);

                if (mode == 0)
                    MPI_Send(buffer, n, MPI_BYTE,
                             0, 0, MPI_COMM_WORLD);

                else if (mode == 1)
                    MPI_Ssend(buffer, n, MPI_BYTE,
                              0, 0, MPI_COMM_WORLD);

                else
                    MPI_Bsend(buffer, n, MPI_BYTE,
                              0, 0, MPI_COMM_WORLD);
            }
        }

        if (rank == 0) {

            double time = MPI_Wtime() - start;

            double time_us =
                time * 1000000.0 / (2.0 * repetitions);

            fprintf(file, "%d,%.9f,%d\n",
                    n, time_us, repetitions);

            printf("%d байт: %.3f мкс\n",
                   n, time_us);
        }
    }

    if (rank == 0)
        fclose(file);

    if (mode == 2) {
        MPI_Buffer_detach(&bsend_buffer, &bsend_size);
    }

    free(buffer);

    MPI_Finalize();

    return 0;
}
