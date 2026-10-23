/*
 * Sequential Sieve of Eratosthenes.
 * Usage: ./crivo_sequencial N
 *
 * Validates N and allocates a zero-filled array before starting the timer.
 * CLOCK_MONOTONIC measures elapsed wall-clock time around the sieve only.
 * Prime counting and writing to primos_linear.txt happen after timing.
 * Normal builds provide the performance baseline; the separate -pg build
 * is used only for profiling, not for speedup measurements.
 */
#define _POSIX_C_SOURCE 200809L

#include "crivo_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    int n = argc == 2 ? parse_limit(argv[1]) : 0;
    if (!n) {
        fprintf(stderr, "Usage: %s N (2 <= N < INT_MAX)\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned char *composite = calloc((size_t)n + 1, 1);
    if (!composite) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        free(composite);
        return EXIT_FAILURE;
    }

    crivo(composite, n);

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        free(composite);
        return EXIT_FAILURE;
    }

    double elapsed = (double)(end.tv_sec - start.tv_sec)
                   + (end.tv_nsec - start.tv_nsec) / 1e9;

    int total = write_primes("primos_linear.txt", composite, n, 0);
    free(composite);
    if (total < 0) return EXIT_FAILURE;

    printf("N=%d primes=%d seconds=%.6f\n", n, total, elapsed);
    return EXIT_SUCCESS;
}
