#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 65536
#endif

#if BLOCK_SIZE < 1
#error BLOCK_SIZE_must_be_positive
#endif

enum { TASK = 10, RESULT = 11, VALUES = 12 };

static void *allocate(size_t count, size_t width) {
    void *ptr = calloc(count ? count : 1, width);
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void crivo(unsigned char *composto, int n) {
    composto[0] = composto[1] = 1;
    for (int p = 2; p <= n / p; p++) {
        if (composto[p] == 0) {
            for (long long m = (long long)p * p; m <= n; m += p) {
                composto[m] = 1;
            }
        }
    }
}

static void send_segment(int low, int high, const int *base, int count) {
    int length = high - low + 1;
    unsigned char *composto = allocate((size_t)length, sizeof(*composto));

    for (int k = 0; k < count; k++) {
        long long p = base[k];
        long long first = ((low + p - 1) / p) * p;
        if (first < p * p) first = p * p;

        for (long long m = first; m <= high; m += p) {
            composto[m - low] = 1;
        }
    }

    int found = 0;
    for (int i = 0; i < length; i++) {
        if (composto[i] == 0) found++;
    }

    int *primes = allocate((size_t)found, sizeof(*primes));
    int used = 0;
    for (int i = 0; i < length; i++) {
        if (composto[i] == 0) primes[used++] = low + i;
    }

    int header[2] = {low, found};
    MPI_Send(header, 2, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    MPI_Send(primes, found, MPI_INT, 0, VALUES, MPI_COMM_WORLD);

    free(primes);
    free(composto);
}

static int receive_segment(unsigned char *primes, int *finished) {
    int header[2];
    MPI_Status status;

    MPI_Recv(header, 2, MPI_INT, MPI_ANY_SOURCE, RESULT,
             MPI_COMM_WORLD, &status);

    int source = status.MPI_SOURCE;
    *finished = (header[0] == 0);

    if (!*finished) {
        int *values = allocate((size_t)header[1], sizeof(*values));

        MPI_Recv(values, header[1], MPI_INT, source, VALUES,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < header[1]; i++) {
            primes[values[i]] = 1;
        }

        free(values);
    }

    return source;
}

static void master(int n, int workers, unsigned char *primes) {
    for (int rank = 1; rank <= workers; rank++) {
        int task[2] = {rank - 1, n};
        MPI_Send(task, 2, MPI_INT, rank, TASK, MPI_COMM_WORLD);
    }

    int remaining = workers;
    while (remaining > 0) {
        int finished;
        receive_segment(primes, &finished);
        if (finished) remaining--;
    }
}

static void worker(int rank, int workers, const int *base, int count) {
    (void)rank;

    int task[2];
    MPI_Recv(task, 2, MPI_INT, 0, TASK,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (long long low = 2LL + (long long)task[0] * BLOCK_SIZE;
         low <= task[1];
         low += (long long)workers * BLOCK_SIZE) {
        long long high = low + BLOCK_SIZE - 1;
        if (high > task[1]) high = task[1];

        send_segment((int)low, (int)high, base, count);
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
                    if (fprintf(file, "%d\n", i) < 0) {
                        result = EXIT_FAILURE;
                        break;
                    }
                }
            }

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
    MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();

    return result;
}
