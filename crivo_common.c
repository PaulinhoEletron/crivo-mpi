#include "crivo_common.h"

void *allocate(size_t count, size_t width) {
    void *ptr = calloc(count ? count : 1, width);
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void crivo(unsigned char *composto, int n) {
// Define a funcao do crivo linear, recebendo um vetor e o limite n superior
	composto[0] = composto[1] = 1;
	for(int p = 2; p <= n/p; p++)  {
		if ((composto[p]) == 0) {
			for (long long m = (long long)p * p; m <= n; m+=p) {
				composto[m] = 1;
			}
		}
	}
}

int write_file(unsigned char *primes, int n){
    FILE *file = fopen("primos.txt", "w");
    if (file == NULL) {
        perror("primos.txt");
        return EXIT_FAILURE;
    } else {

        for (int i = 2; i <= n; i++) {
            if (primes[i]) {
                if (fprintf(file, "%d ", i) < 0) {
                    return EXIT_FAILURE;
                }
            }
        }
        fprintf(file, "\n");

        if (fclose(file) != 0) return EXIT_FAILURE;
    }

}

void send_segment(int low, int high, const int *base, int count) {
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

int receive_segment(unsigned char *primes, int *finished) {
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
