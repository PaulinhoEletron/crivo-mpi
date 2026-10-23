/* Shared interface for the sequential and MPI programs.
 * crivo marks non-primes with 1 in a zero-filled array of n+1 bytes.
 * parse_limit validates the command-line limit; write_primes writes the
 * flagged primes in ascending order. prime_flag selects which flag means
 * prime: 0 for the sequential sieve, 1 for the MPI result bitmap.
 * This module has no MPI dependency.
 */
#ifndef CRIVO_COMMON_H
#define CRIVO_COMMON_H

int parse_limit(const char *text);
void crivo(unsigned char *composite, int n);
int write_primes(const char *path, const unsigned char *flags,
                 int n, unsigned char prime_flag);

#endif
