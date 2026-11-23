 -N life_vdi_submit
#PBS -q batch
#PBS -l walltime=00:10:00
#PBS -l nodes=4:ppn=4

cd $PBS_O_WORKDIR
module load mpi
mpicc -std=c99 -O2 -Wall -Wextra life_mpi.c -o life_mpi

mpirun --hostfile $PBS_NODEFILE -np 1 ./life_mpi p46gun.cfg > result_vdi_np1.txt
mpirun --hostfile $PBS_NODEFILE -np 2 ./life_mpi p46gun.cfg > result_vdi_np2.txt
mpirun --hostfile $PBS_NODEFILE -np 4 ./life_mpi p46gun.cfg > result_vdi_np4.txt
mpirun --hostfile $PBS_NODEFILE -np 8 ./life_mpi p46gun.cfg > result_vdi_np8.txt
mpirun --hostfile $PBS_NODEFILE -np 16 ./life_mpi p46gun.cfg > result_vdi_np16.txt
