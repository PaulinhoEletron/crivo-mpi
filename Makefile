ARG := $(word 2, $(MAKECMDGOALS))
$(eval $(ARG):;@:)

seq:
	gcc crivo_sequencial.c -o crivo_sequencial
	./crivo_sequencial $(ARG)
	cat primos_linear.txt

seqrun:
	./crivo_sequencial $(ARG)
	cat primos_linear.txt
