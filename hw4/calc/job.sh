#!/bin/bash
#SBATCH -p AMD9554
#SBATCH -N 1
#SBATCH -n 2
#SBATCH -t 00:05:00
#SBATCH -o cols-%j.out

cd "$SLURM_SUBMIT_DIR"
module load mpi/latest
mpirun -np 2 ./life_cols big_field.cfg
