#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int nx, ny;
	int steps;
	int rank, size;
	int local_nx;
	int stride;
	int *u0, *u1;
	MPI_Datatype column;
} Life;

static int width(int nx, int size, int rank) {
	return nx / size + (rank < nx % size);
}

static int first_col(int nx, int size, int rank) {
	int rem = nx % size;
	return rank * (nx / size) + (rank < rem ? rank : rem);
}

static size_t idx(const Life *l, int x, int y) {
	return y * l->stride + x;
}

static int *new_field(int rows, int cols) {
	int *a = calloc(rows * cols, sizeof *a);
	return a;
}

static MPI_Datatype make_strip(int rows, int block, int stride) {
	MPI_Datatype type;
	MPI_Type_vector(rows, block, stride, MPI_INT, &type);
	MPI_Type_commit(&type);
	return type;
}

static void life_init(Life *l, const char *path) {
	int params[3] = {0};
	FILE *f = NULL;

	if (l->rank == 0) {
		int save_steps;
		f = fopen(path, "r");

		fscanf(f, "%d%d%d%d", &params[0], &save_steps, &params[1], &params[2]);
	}

	MPI_Bcast(params, 3, MPI_INT, 0, MPI_COMM_WORLD);
	l->steps = params[0];
	l->nx = params[1];
	l->ny = params[2];

	l->local_nx = width(l->nx, l->size, l->rank);
	l->stride = l->local_nx + 2;
	l->u0 = new_field(l->ny, l->stride);
	l->u1 = new_field(l->ny, l->stride);

	l->column = make_strip(l->ny, 1, l->stride);

	if (l->rank == 0) {
		int *global = new_field(l->ny, l->nx);
		int x, y, count;
		while ((count = fscanf(f, "%d%d", &x, &y)) == 2) {
			x = (x % l->nx + l->nx) % l->nx;
			y = (y % l->ny + l->ny) % l->ny;
			global[y * l->nx + x] = 1;
		}
		fclose(f);

		for (y = 0; y < l->ny; ++y) {
			memcpy(l->u0 + idx(l, 1, y), global + y * l->nx, l->local_nx * sizeof(int));
		}

		for (int r = 1; r < l->size; ++r) {
			int w = width(l->nx, l->size, r);
			int start = first_col(l->nx, l->size, r);
			MPI_Datatype strip = make_strip(l->ny, w, l->nx);
			MPI_Send(global + start, 1, strip, r, 10, MPI_COMM_WORLD);
			MPI_Type_free(&strip);
		}
		free(global);
	} else {
		MPI_Datatype inside = make_strip(l->ny, l->local_nx, l->stride);
		MPI_Recv(l->u0 + 1, 1, inside, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Type_free(&inside);
	}
}

static void life_step(Life *l) {
	int left = (l->rank - 1 + l->size) % l->size;
	int right = (l->rank + 1) % l->size;
	int w = l->local_nx;

	MPI_Sendrecv(l->u0 + w, 1, l->column, right, 0, l->u0, 1, l->column, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Sendrecv(l->u0 + 1, 1, l->column, left, 1, l->u0 + w + 1, 1, l->column, right, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	for (int y = 0; y < l->ny; ++y) {
		int above = (y == 0) ? l->ny - 1 : y - 1;
		int below = (y == l->ny - 1) ? 0 : y + 1;

		const int *a = l->u0 + idx(l, 0, above);
		const int *b = l->u0 + idx(l, 0, y);
		const int *c = l->u0 + idx(l, 0, below);
		int *next = l->u1 + idx(l, 0, y);

		for (int x = 1; x <= w; ++x) {
			int n = a[x - 1] + a[x] + a[x + 1] + b[x - 1] + b[x + 1] + c[x - 1] + c[x] + c[x + 1];
			next[x] = (n == 3 || (b[x] && n == 2));
		}
	}

	int *tmp = l->u0;
	l->u0 = l->u1;
	l->u1 = tmp;
}

static void dump_field(const Life *l) {
	if (l->rank == 0) {
		int *global = new_field(l->ny, l->nx);
		for (int y = 0; y < l->ny; ++y) {
			memcpy(global + y * l->nx, l->u0 + idx(l, 1, y), l->local_nx * sizeof(int));
		}

		for (int r = 1; r < l->size; ++r) {
			int start = first_col(l->nx, l->size, r);
			MPI_Datatype strip = make_strip(l->ny, width(l->nx, l->size, r), l->nx);
			MPI_Recv(global + start, 1, strip, r, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Type_free(&strip);
		}

		for (int y = 0; y < l->ny; ++y) {
			for (int x = 0; x < l->nx; ++x) {
				if (global[y * l->nx + x]) printf("CELL %d %d\n", x, y);
			}
		}
		free(global);
	} else {
		MPI_Datatype inside = make_strip(l->ny, l->local_nx, l->stride);
		MPI_Send(l->u0 + 1, 1, inside, 0, 20, MPI_COMM_WORLD);
		MPI_Type_free(&inside);
	}
}

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);
	Life l = {0};
	MPI_Comm_rank(MPI_COMM_WORLD, &l.rank);
	MPI_Comm_size(MPI_COMM_WORLD, &l.size);

	life_init(&l, argv[1]);

	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();
	for (int i = 0; i < l.steps; ++i) {
		life_step(&l);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	double elapsed = MPI_Wtime() - start;

	if (l.rank == 0) printf("TIME RESULT: %.6f sec\n", elapsed);

	if (argc == 3) dump_field(&l);

	MPI_Type_free(&l.column);
	free(l.u0);
	free(l.u1);
	MPI_Finalize();
	return 0;
}
