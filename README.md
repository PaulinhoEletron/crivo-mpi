# MPI Prime Sieve

## Files

- `crivo_sequencial.c`: sequential sieve and execution timing.
- `crivo_mpi.c`: contiguous, cyclic and dynamic MPI strategies.
- `crivo_common.c`: shared sieve, input and output functions.
- `crivo_common.h`: shared declarations.
- `Makefile`: compilation, execution and profiling.

## Run

Requires GCC, Make, Open MPI and gprof. From the source directory:

```bash
make
make seq N=10000000
make cnt N=10000000 NP=2
make cic N=10000000 NP=2
make din N=10000000 NP=2
make profile N=10000000
```

Vary `N` and `NP` to compare execution times. Repeat each configuration five times and use its median. Speedup = sequential time / parallel time; efficiency = speedup / NP × 100%.

Outputs: execution times in the terminal, primes in `primos_linear.txt` and `primos.txt`, and profiling in `gprof_report.txt`. Each run overwrites its corresponding files.
