#include "crivo_common.h"

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

    for(int i = 0; i <=n; i++)
        composto[i] = 1 - composto[i];

    if(write_file(composto, n) == EXIT_FAILURE){
        fprintf(stderr, "Error ao gravar primos_linear.txt.\n");
        return EXIT_FAILURE;
    }

    free(composto);

    return EXIT_SUCCESS;
}





