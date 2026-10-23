/* MPI Sieve of Eratosthenes with three scheduling strategies.
 * Usage: mpirun -np P ./crivo_mpi N contiguo|ciclico|dinamico
 * Rank 0 generates and broadcasts primes through sqrt(N), assigns work,
 * collects actual prime lists, and writes sorted results to primos.txt.
 * contiguo: one approximately equal interval per worker.
 * ciclico: fixed blocks assigned in round-robin order.
 * dinamico: the next block goes to the worker that just returned a result.
 * There are P-1 sieve workers; rank 0 is the coordinator. Every process has
 * private memory. TASK carries an inclusive interval (or cyclic block ID
 * and N). RESULT carries [low,count]; [0,0] means worker finished. VALUES
 * carries the actual primes. A normal empty result still sends VALUES.
 * The master's timer includes base-prime generation, communication, sieving,
 * and collection, but excludes output writing and its initial bitmap allocation.
 * BLOCK_SIZE can be overridden at compile time with -DBLOCK_SIZE=value.
 */
#include "crivo_common.h"
#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 65536
#endif
#if BLOCK_SIZE < 1
#error BLOCK_SIZE_must_be_positive
#endif

enum { CONTIGUO, CICLICO, DINAMICO };
enum { TASK = 10, RESULT, VALUES };

static void *allocate(size_t count, size_t width) {
    void *p = calloc(count ? count : 1, width);
    if (!p) {
        fprintf(stderr, "Memory allocation failed.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
    return p;
}

static void send_segment(int low, int high, const int *base, int count) {
    int length = high - low + 1, found = 0;
    unsigned char *composite = allocate((size_t)length, 1);
    for (int k = 0; k < count; k++) {
        long long p = base[k], first = ((low + p - 1) / p) * p;
        if (first < p * p) first = p * p;
        for (long long m = first; m <= high; m += p) composite[m - low] = 1;
    }
    for (int i = 0; i < length; i++) if (!composite[i]) found++;
    int *values = allocate((size_t)found, sizeof(*values));
    int used = 0;
    for (int i = 0; i < length; i++) if (!composite[i]) values[used++] = low + i;
    int header[2] = {low, found};
    MPI_Send(header, 2, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    MPI_Send(values, found, MPI_INT, 0, VALUES, MPI_COMM_WORLD);
    free(values);
    free(composite);
}

static void dispatch(int rank, int n, long long *next) {
    int task[2] = {0, 0};
    if (*next <= n) {
        long long high = *next + BLOCK_SIZE - 1;
        if (high > n) high = n;
        task[0] = (int)*next;
        task[1] = (int)high;
        *next = high + 1;
    }
    MPI_Send(task, 2, MPI_INT, rank, TASK, MPI_COMM_WORLD);
}

static void master(int n, int workers, int mode, unsigned char *primes) {
    long long next = 2;
    for (int rank = 1; rank <= workers; rank++) {
        if (mode == DINAMICO) { dispatch(rank, n, &next); continue; }
        int task[2] = {rank - 1, n};
        if (mode == CONTIGUO) {
            task[0] = 2 + (int)((long long)(n - 1) * (rank - 1) / workers);
            task[1] = 1 + (int)((long long)(n - 1) * rank / workers);
            if (task[0] > task[1]) task[0] = task[1] = 0;
        }
        MPI_Send(task, 2, MPI_INT, rank, TASK, MPI_COMM_WORLD);
    }
    int remaining = workers;
    while (remaining) {
        int header[2];
        MPI_Status status;
        MPI_Recv(header, 2, MPI_INT, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
        int source = status.MPI_SOURCE;
        if (!header[0]) { remaining--; continue; }
        int *values = allocate((size_t)header[1], sizeof(*values));
        MPI_Recv(values, header[1], MPI_INT, source, VALUES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < header[1]; i++) primes[values[i]] = 1;
        free(values);
        if (mode == DINAMICO) dispatch(source, n, &next);
    }
}

static void contiguous_worker(const int *task, const int *base, int count) {
    if (task[0]) send_segment(task[0], task[1], base, count);
}

static void cyclic_worker(const int *task, int workers, const int *base, int count) {
    for (long long low = 2LL + (long long)task[0] * BLOCK_SIZE;
         low <= task[1]; low += (long long)workers * BLOCK_SIZE) {
        long long high = low + BLOCK_SIZE - 1;
        if (high > task[1]) high = task[1];
        send_segment((int)low, (int)high, base, count);
    }
}

static void dynamic_worker(int *task, const int *base, int count) {
    while (task[0]) {
        send_segment(task[0], task[1], base, count);
        MPI_Recv(task, 2, MPI_INT, 0, TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size, config[2] = {0, -1};
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0 && argc == 3 && size >= 2) {
        const char *names[] = {"contiguo", "ciclico", "dinamico"};
        config[0] = parse_limit(argv[1]);
        for (int i = 0; i < 3; i++) if (!strcmp(argv[2], names[i])) config[1] = i;
    }
    MPI_Bcast(config, 2, MPI_INT, 0, MPI_COMM_WORLD);
    int n = config[0], mode = config[1];
    if (!n || mode < 0) {
        if (rank == 0) fprintf(stderr, "Usage: mpirun -np P %s N contiguo|ciclico|dinamico (P >= 2, 2 <= N < INT_MAX)\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    unsigned char *primes = rank == 0 ? allocate((size_t)n + 1, 1) : NULL;
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();
    int root = (int)sqrt((double)n), count = 0;
    int *base = allocate((size_t)root + 1, sizeof(*base));
    if (rank == 0) {
        unsigned char *small = allocate((size_t)root + 1, 1);
        if (root >= 2) crivo(small, root);
        for (int p = 2; p <= root; p++) if (!small[p]) base[count++] = p;
        free(small);
    }
    MPI_Bcast(&count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(base, count, MPI_INT, 0, MPI_COMM_WORLD);
    int result = EXIT_SUCCESS;
    if (rank == 0) {
        master(n, size - 1, mode, primes);
        double elapsed = MPI_Wtime() - start;
        int total = write_primes("primos.txt", primes, n, 1);
        if (total < 0) result = EXIT_FAILURE;
        else printf("N=%d processes=%d workers=%d primes=%d seconds=%.6f\n", n, size, size - 1, total, elapsed);
        free(primes);
    } else {
        int task[2];
        MPI_Recv(task, 2, MPI_INT, 0, TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (mode == CONTIGUO) contiguous_worker(task, base, count);
        else if (mode == CICLICO) cyclic_worker(task, size - 1, base, count);
        else dynamic_worker(task, base, count);
        int done[2] = {0, 0};
        MPI_Send(done, 2, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    }
    free(base);
    MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return result;
}
