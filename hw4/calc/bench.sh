#!/bin/bash
#SBATCH -p AMD9554
#SBATCH -N 1
#SBATCH -n 32
#SBATCH -t 00:30:00
#SBATCH -o bench-%j.out

set -e
cd "$SLURM_SUBMIT_DIR"
module load mpi/latest

mpicc -std=c99 -O2 life_rows.c -o life_rows
mpicc -std=c99 -O2 life_cols.c -o life_cols

mkdir -p "logs_${SLURM_JOB_ID}"
csv="times_calc_${SLURM_JOB_ID}.csv"
echo "version,processes,repeat,time_s" > "$csv"

for version in rows cols; do
    for p in 1 2 4 8 16 32; do
        for repeat in 1 2 3; do
            log="logs_${SLURM_JOB_ID}/${version}_${p}_${repeat}.out"
            echo "Running: $version, processes=$p, repeat=$repeat"

            if timeout 60s mpirun -np "$p" \
                "./life_${version}" big_field.cfg > "$log" 2>&1; then

                time=$(awk '/TIME[_ ]RESULT:/ {print $(NF-1)}' "$log")

                if [ -z "$time" ]; then
                    echo "No time found. See $log"
                    exit 1
                fi

                echo "$version,$p,$repeat,$time" >> "$csv"
            else
                echo "Run failed or exceeded 60 seconds. See $log"
                exit 1
            fi
        done
    done
done

cat "$csv"
