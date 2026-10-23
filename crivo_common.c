/* Common input, sieve, and output functions.
 * The sieve starts marking at p*p and stops trying divisors at sqrt(n).
 * p <= n/p avoids overflow in that loop condition. Buffers must contain
 * at least n+1 bytes and be zero-initialized before calling crivo (n >= 2).
 * parse_limit returns 0 for invalid input. write_primes returns the prime
 * count, or -1 on failure; output uses spaces and a final newline.
 */
#include "crivo_common.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int parse_limit(const char *text) {
    char *end;
    errno = 0;
    long n = strtol(text, &end, 10);
    return errno || end == text || *end || n < 2 || n >= INT_MAX ? 0 : (int)n;
}

void crivo(unsigned char *composite, int n) {
    composite[0] = composite[1] = 1;
    for (int p = 2; p <= n / p; p++) {
        if (!composite[p]) {
            for (long long m = (long long)p * p; m <= n; m += p)
                composite[m] = 1;
        }
    }
}

int write_primes(const char *path, const unsigned char *flags,
                 int n, unsigned char prime_flag) {
    FILE *file = fopen(path, "w");
    if (!file) { perror(path); return -1; }
    int total = 0, failed = 0;
    for (int i = 2; i <= n; i++) {
        if (flags[i] == prime_flag) {
            if (fprintf(file, "%d ", i) < 0) { failed = 1; break; }
            total++;
        }
    }
    if (fputc('\n', file) == EOF) failed = 1;
    if (fclose(file) != 0) failed = 1;
    if (failed) fprintf(stderr, "Failed to write %s.\n", path);
    return failed ? -1 : total;
}
