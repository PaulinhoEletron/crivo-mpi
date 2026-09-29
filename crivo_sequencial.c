#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

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

int main(int argc, char *argv[]){
// Começa o programa, argc conta os argumentos e argv guarda os dados
	if (argc != 2) {
		fprintf(stderr, "Uso: %s N\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *fim;
	errno = 0;
	long valor = strtol(argv[1], &fim, 10);
	/* declara um ponteiro onde a conversao de numeros parou, errno limpa qualquer
	erros antes de chamar strtol, &fim atualiza fim para apontar pra primeiro caractere 
	nao convertido*/  
	
	// Checa por erros de conversao, digitos faltando ou caracteres sobrando 
	if (errno || fim == argv[1] || *fim != '\0' || valor < 2 || valor >= INT_MAX){
		fprintf(stderr, "N deve estar entre 2 e %d.\n", INT_MAX -1);
		return EXIT_FAILURE;
	}
	
	int n = (int)valor;
	// converte o valor validado para inteiro
	unsigned char *composto = calloc((size_t)n + 1, sizeof(*composto));
	// Aloca n+1 elementos do vetor, dos indicis de 0 a n
	if ((composto) == NULL) {
		fprintf(stderr, "Memoria insuficiente.\n");
		return EXIT_FAILURE;
	}
	
	crivo(composto, n);
	// chamado do crivo linear
	FILE * saida = fopen("primos_linear.txt", "w");
	if (saida == NULL) {
		perror("primos_linear.txt");
		free(composto);
		return EXIT_FAILURE;
	}
	// termina a checagem de abertura
	
	for (int i = 2; i <= n; i++) {
		if ( (composto[i]) == 0) {
			if(fprintf(saida, "%d ", i) < 0) {
				break;
			}
		}
	}
	// termina a leitura do vetor
	
	int error = ferror(saida);
	if(fclose(saida) != 0) {
		error = 1;
	}
	// termina a checagem de fechamento
	
	free(composto);
	
	if(error) {
	//checa se alguem error de output foi salvo
		fprintf(stderr, "Error ao gravar primos_linear.txt.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
	
	
	
	
		
