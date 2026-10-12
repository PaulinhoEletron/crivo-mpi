#include "crivo_common.h"

static void make_cuts(int n, int workers, int *cuts) {
    for (int i = 0; i <= workers; i++) {
        cuts[i] = 2 + (int)(((long long)n - 1) * i / workers);
    }
}

static void master(int n, int workers, unsigned char *primes) {
    int *cuts = allocate((size_t)workers + 1, sizeof(*cuts));
    make_cuts(n, workers, cuts);

    for (int rank = 1; rank <= workers; rank++) {
        int task[2] = {cuts[rank - 1], cuts[rank] - 1};
        if (task[0] > task[1]) task[0] = task[1] = 0;
        MPI_Send(task, 2, MPI_INT, rank, TASK, MPI_COMM_WORLD);
    }

    free(cuts);

    int remaining = workers;
    while (remaining > 0) {
        int finished;
        receive_segment(primes, &finished);
        if (finished) remaining--;
    }
}

static void worker(int rank, int workers, const int *base, int count) {
    (void)rank;
    (void)workers;

    int task[2];
    MPI_Recv(task, 2, MPI_INT, 0, TASK,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (task[0] != 0) {
        send_segment(task[0], task[1], base, count);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 0;
    if (rank == 0) {
        if (argc == 2 && size >= 2) {
            char *end;
            errno = 0;
            long value = strtol(argv[1], &end, 10);

            if (!errno && end != argv[1] && *end == '\0' &&
                value >= 2 && value < INT_MAX) {
                n = (int)value;
            }
        }

        if (n == 0) {
            fprintf(stderr,
                    "Usage: mpirun -np P %s N "
                    "(P >= 2, 2 <= N < INT_MAX)\n", argv[0]);
        }
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (n == 0) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    unsigned char *primes = NULL;
    if (rank == 0) {
        primes = allocate((size_t)n + 1, sizeof(*primes));
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = 0.0;
    if (rank == 0) start = MPI_Wtime();

    int root = (int)sqrt((double)n);
    int *base = allocate((size_t)root + 1, sizeof(*base));
    int count = 0;

    if (rank == 0) {
        unsigned char *small = allocate((size_t)root + 1, sizeof(*small));
        crivo(small, root);

        for (int p = 2; p <= root; p++) {
            if (small[p] == 0) base[count++] = p;
        }

        free(small);
    }

    MPI_Bcast(&count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(base, count, MPI_INT, 0, MPI_COMM_WORLD);

    int result = EXIT_SUCCESS;
    if (rank == 0) {
        master(n, size - 1, primes);
        double elapsed = MPI_Wtime() - start;

        FILE *file = fopen("primos.txt", "w");
        if (file == NULL) {
            perror("primos.txt");
            result = EXIT_FAILURE;
        } else {
            int total = 0;

            for (int i = 2; i <= n; i++) {
                if (primes[i]) {
                    total++;
                    if (fprintf(file, "%d ", i) < 0) {
                        result = EXIT_FAILURE;
                        break;
                    }
                }
            }
            fprintf(file, "\n");

            if (fclose(file) != 0) result = EXIT_FAILURE;

            if (result == EXIT_SUCCESS) {
                printf("N=%d processes=%d workers=%d primes=%d seconds=%.6f\n",
                       n, size, size - 1, total, elapsed);
            } else {
                fprintf(stderr, "Failed to write primos.txt.\n");
            }
        }

        free(primes);
    } else {
        worker(rank, size - 1, base, count);
        int done[2] = {0, 0};
        MPI_Send(done, 2, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    }

    free(base);
    MPI_Finalize();

    return result;
}
