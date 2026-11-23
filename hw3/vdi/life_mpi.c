#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <mpi.h>

#define ind_global(i, j, nx, ny) (((i + nx) % nx) + ((j + ny) % ny) * (nx))
#define ind_local(i, j, nx) (((i + nx) % nx) + (j) * (nx))

typedef struct {
	int nx, ny;
	int local_ny;
	int *u0;
	int *u1;
	int steps;
	int save_steps;
	int rank, size;
} life_t;

void life_init(const char *path, life_t *l);
void life_free(life_t *l);
void life_step(life_t *l);
void life_save_vtk(const char *path, life_t *l);

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    
    life_t l;
    MPI_Comm_rank(MPI_COMM_WORLD, &l.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &l.size);

    if (argc != 2) {
        if (l.rank == 0) printf("Usage: %s input file.\n", argv[0]);
        MPI_Finalize();
        return 0;
    }

    life_init(argv[1], &l);
    
    double start_time, end_time;
    
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    int i;
    for (i = 0; i < l.steps; i++) {
        life_step(&l);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    if (l.rank == 0) {
        printf("TIME_RESULT: %f seconds\n", end_time - start_time);
    }
    
    life_free(&l);
    MPI_Finalize();
    return 0;
}

/**
 * Загрузить входную конфигурацию.
 * Формат файла, число шагов, как часто сохранять, размер поля, затем идут координаты заполненых клеток:
 * steps
 * save_steps
 * nx ny
 * i1 j2
 * i2 j2
 */
void life_init(const char *path, life_t *l) {
    if (l->rank == 0) {
        FILE *fd = fopen(path, "r");
        assert(fd);
        assert(fscanf(fd, "%d\n", &l->steps) == 1);
        assert(fscanf(fd, "%d\n", &l->save_steps) == 1);
        assert(fscanf(fd, "%d %d\n", &l->nx, &l->ny) == 2);
        fclose(fd);
        printf("Steps %d, save every %d step.\n", l->steps, l->save_steps);
        printf("Field size: %dx%d\n", l->nx, l->ny);
    }

    MPI_Bcast(&l->steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&l->save_steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&l->nx, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&l->ny, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (l->ny % l->size != 0) {
        if (l->rank == 0) printf("Error: ny (%d) must be divisible by size (%d)\n", l->ny, l->size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    l->local_ny = l->ny / l->size;

    int local_size = l->nx * (l->local_ny + 2);
    l->u0 = (int*)calloc(local_size, sizeof(int));
    l->u1 = (int*)calloc(local_size, sizeof(int));

    int *global_u0 = NULL;
    if (l->rank == 0) {
        global_u0 = (int*)calloc(l->nx * l->ny, sizeof(int));
        FILE *fd = fopen(path, "r");
        assert(fd);
        
        int tmp_steps, tmp_save, tmp_nx, tmp_ny;
        fscanf(fd, "%d\n", &tmp_steps);
        fscanf(fd, "%d\n", &tmp_save);
        fscanf(fd, "%d %d\n", &tmp_nx, &tmp_ny);

        int i, j, cnt = 0;
        while (fscanf(fd, "%d %d\n", &i, &j) == 2) {
            i = (i % l->nx + l->nx) % l->nx;
            j = (j % l->ny + l->ny) % l->ny;
            global_u0[ind_global(i, j, l->nx, l->ny)] = 1;
            cnt++;
        }
        printf("Loaded %d life cells.\n", cnt);
        fclose(fd);
    }

    MPI_Scatter(global_u0, l->local_ny * l->nx, MPI_INT,
                l->u0 + l->nx, l->local_ny * l->nx, MPI_INT,
                0, MPI_COMM_WORLD);

    if (l->rank == 0) free(global_u0);
}


void life_free(life_t *l)
{
	free(l->u0);
	free(l->u1);
	l->nx = l->ny = 0;
}

void life_save_vtk(const char *path, life_t *l) {
    int *global_u0 = NULL;
    if (l->rank == 0) {
        global_u0 = (int *)malloc(l->nx * l->ny * sizeof(int));
    }
    
    MPI_Gather(l->u0 + l->nx, l->nx * l->local_ny, MPI_INT,
               global_u0, l->nx * l->local_ny, MPI_INT,
               0, MPI_COMM_WORLD);

    if (l->rank == 0) {
        FILE *f = fopen(path, "w");
        assert(f);
        fprintf(f, "# vtk DataFile Version 3.0\n");
        fprintf(f, "Created by MPI Life\n");
        fprintf(f, "ASCII\n");
        fprintf(f, "DATASET STRUCTURED_POINTS\n");
        fprintf(f, "DIMENSIONS %d %d 1\n", l->nx + 1, l->ny + 1);
        fprintf(f, "SPACING %d %d 0.0\n", 1, 1);
        fprintf(f, "ORIGIN %d %d 0.0\n", 0, 0);
        fprintf(f, "CELL_DATA %d\n", l->nx * l->ny);
        fprintf(f, "SCALARS life int 1\n");
        fprintf(f, "LOOKUP_TABLE life_table\n");
        
        for (int j = 0; j < l->ny; j++) {
            for (int i = 0; i < l->nx; i++) {
                fprintf(f, "%d\n", global_u0[ind_global(i, j, l->nx, l->ny)]);
            }
        }
        fclose(f);
        free(global_u0);
    }
}

void life_step(life_t *l) {
    int top = (l->rank - 1 + l->size) % l->size;
    int bottom = (l->rank + 1) % l->size;
    int nx = l->nx;
    int local_ny = l->local_ny;

    if (l->rank % 2 == 0) {
        MPI_Send(l->u0 + local_ny * nx, nx, MPI_INT, bottom, 0, MPI_COMM_WORLD);
        MPI_Recv(l->u0, nx, MPI_INT, top, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(l->u0, nx, MPI_INT, top, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(l->u0 + local_ny * nx, nx, MPI_INT, bottom, 0, MPI_COMM_WORLD);
    }

    if (l->rank % 2 == 0) {
        MPI_Send(l->u0 + nx, nx, MPI_INT, top, 1, MPI_COMM_WORLD);
        MPI_Recv(l->u0 + (local_ny + 1) * nx, nx, MPI_INT, bottom, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(l->u0 + (local_ny + 1) * nx, nx, MPI_INT, bottom, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(l->u0 + nx, nx, MPI_INT, top, 1, MPI_COMM_WORLD);
    }

    for (int j = 1; j <= local_ny; j++) {
        for (int i = 0; i < nx; i++) {
            int n = 0;
            int ip = (i + 1) % nx;
            int im = (i - 1 + nx) % nx;

            n += l->u0[ind_local(ip, j - 1, nx)];
            n += l->u0[ind_local(i,  j - 1, nx)];
            n += l->u0[ind_local(im, j - 1, nx)];
            n += l->u0[ind_local(ip, j,     nx)];
            n += l->u0[ind_local(im, j,     nx)];
            n += l->u0[ind_local(ip, j + 1, nx)];
            n += l->u0[ind_local(i,  j + 1, nx)];
            n += l->u0[ind_local(im, j + 1, nx)];

            int current = l->u0[ind_local(i, j, nx)];
            if (current == 0 && n == 3) {
                l->u1[ind_local(i, j, nx)] = 1;
            } else if (current == 1 && (n == 2 || n == 3)) {
                l->u1[ind_local(i, j, nx)] = 1;
            } else {
                l->u1[ind_local(i, j, nx)] = 0;
            }
        }
    }

    int *tmp = l->u0;
    l->u0 = l->u1;
    l->u1 = tmp;
}
