# Build the sequential executable with gcc and the unified MPI executable
# with mpicc. Select N, NP (total processes), and STRATEGY on the command line:
# make run N=10000000 NP=2 STRATEGY=contiguo
# make seq N=10000000
# make profile N=10000000
# For extra processes on a small laptop, set MPI_FLAGS=--oversubscribe.
# Source/header dependencies rebuild changed programs automatically.
# Profiling uses a separate executable so normal builds remain uninstrumented.

CC = gcc
MPICC = mpicc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic
N = 10000000
NP = 2
STRATEGY = contiguo
MPI_FLAGS =

.PHONY: all run seq cnt cic din profile clean
all: crivo_sequencial crivo_mpi

crivo_sequencial: crivo_sequencial.c crivo_common.c crivo_common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) crivo_sequencial.c crivo_common.c $(LDFLAGS) -o $@

crivo_mpi: crivo_mpi.c crivo_common.c crivo_common.h
	$(MPICC) $(CPPFLAGS) $(CFLAGS) crivo_mpi.c crivo_common.c $(LDFLAGS) -o $@ -lm

seq: crivo_sequencial
	./crivo_sequencial $(N)

run: crivo_mpi
	mpirun $(MPI_FLAGS) -np $(NP) ./crivo_mpi $(N) $(STRATEGY)

cnt: STRATEGY = contiguo
cic: STRATEGY = ciclico
din: STRATEGY = dinamico
cnt cic din: run

crivo_profile: crivo_sequencial.c crivo_common.c crivo_common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -pg crivo_sequencial.c crivo_common.c $(LDFLAGS) -o $@

profile: crivo_profile
	./crivo_profile $(N)
	gprof ./crivo_profile gmon.out > gprof_report.txt

clean:
	rm -f crivo_sequencial crivo_mpi crivo_profile gmon.out
