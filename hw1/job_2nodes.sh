#!/bin/bash
#PBS -l walltime=00:05:00,nodes=2:ppn=1
#PBS -N hw1_2nodes
#PBS -q batch

cd $PBS_O_WORKDIR

mpirun --hostfile $PBS_NODEFILE -np 2 ./main send send_2nodes.csv
mpirun --hostfile $PBS_NODEFILE -np 2 ./main ssend ssend_2nodes.csv
mpirun --hostfile $PBS_NODEFILE -np 2 ./main bsend bsend_2nodes.csv

rm -f hw1_2nodes.o* hw1_2nodes.e*
