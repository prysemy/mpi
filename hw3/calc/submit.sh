#!/bin/bash

for np in 1 2 4 8 16 32 64; do
    cat <<EOF > job_np${np}.sh
#!/bin/bash
#SBATCH -J life_bench_${np}
#SBATCH -n ${np}
#SBATCH -t 00:10:00
#SBATCH --partition=AMD9554

module load mpi/latest
mpirun -np ${np} ./life_mpi big_field.cfg
EOF
    sbatch job_np${np}.sh
done
