#!/bin/bash
#SBATCH -J life_mpi 
#SBATCH -n 2 
#SBATCH -t 00:05:00  
#SBATCH --partition=AMD9554

module load mpi/latest
mpirun -np 2 ./life_mpi glider_10x10.cfg
