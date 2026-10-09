#ifndef CRIVO_COMMON_H
#define CRIVO_COMMON_H

#include <stddef.h>

enum { TASK = 10, RESULT = 11, VALUES = 12 };

void *allocate(size_t count, size_t width);
void crivo(unsigned char *composto, int n);
void send_segment(int low, int high, const int *base, int count);
int receive_segment(unsigned char *primes, int *finished);

#endif
