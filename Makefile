ARG_RAW := $(word 2, $(MAKECMDGOALS))
NP_RAW  := $(word 3, $(MAKECMDGOALS))

ifneq ($(ARG_RAW),)
  $(eval $(ARG_RAW):;@:)
endif
ifneq ($(NP_RAW),)
  $(eval $(NP_RAW):;@:)
endif

ARG := $(if $(ARG_RAW),$(ARG_RAW),3000)
NP  := $(if $(NP_RAW),$(NP_RAW),4)

USE_HW := $(shell [ $$(($(NP) + 0)) -lt 5 ] && echo "no" || echo "yes")

ifeq ($(USE_HW),yes)
    MPI_FLAGS := --use-hwthread-cpus
else
    MPI_FLAGS :=
endif

seq:
	mpicc crivo_sequencial.c crivo_common.c -o crivo_sequencial
	./crivo_sequencial $(ARG)
	#cat primos_linear.txt

seqrun:
	./crivo_sequencial $(ARG)
	#cat primos_linear.txt

cic:
	mpicc crivo_mpi_ciclico.c crivo_common.c -o crivo_ciclico -lm
	mpirun $(MPI_FLAGS) -np $(NP) ./crivo_ciclico $(ARG)
	#cat primos.txt

cicrun:
	mpirun $(MPI_FLAGS) -np $(NP) ./crivo_ciclico $(ARG)
	#cat primos.txt
