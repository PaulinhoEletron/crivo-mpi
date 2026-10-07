ARG := $(word 3, $(MAKECMDGOALS))
NP  := $(word 2, $(MAKECMDGOALS))

$(eval $(ARG):;@:)
$(eval $(NP):;@:)


ARG ?= 3000
NP  ?= 4

USE_HW := $(shell [ $(NP) -lt 5 ] && echo "no" || echo "yes")

ifeq ($(USE_HW),yes)
    MPI_FLAGS := --use-hwthread-cpus
else
    MPI_FLAGS :=
endif

seq:
	gcc crivo_sequencial.c -o crivo_sequencial
	./crivo_sequencial $(ARG)
	cat primos_linear.txt

seqrun:
	./crivo_sequencial $(ARG)
	cat primos_linear.txt

cic:
	mpicc crivo_mpi_ciclico.c -o crivo_ciclico -lm
	mpirun $(MPI_FLAGS) -np $(NP) ./crivo_ciclico $(ARG)
	cat primos.txt

cicrun:
	mpirun $(MPI_FLAGS) -np $(NP) ./crivo_ciclico $(ARG)
	cat primos.txt
