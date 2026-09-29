#!/bin/bash

sizes=(4800 6400 8192 9600 12288 16384 20480 25600)

OUTPUT_MPI="times_mpi_nfs.csv"

echo -n "Run" > $OUTPUT_MPI
for s in "${sizes[@]}"; do
    echo -n ",$s" >> $OUTPUT_MPI
done
echo "" >> $OUTPUT_MPI

for j in {1..10}; do
    echo -n "$j" >> $OUTPUT_MPI

    for s in "${sizes[@]}"; do

        time_mpi=$(mpirun -np 32 --hostfile hosts --map-by node ./mainNfs $s | grep "Execution time" | awk '{print $3}')

        echo -n ",$time_mpi" >> $OUTPUT_MPI
    done

    echo "" >> $OUTPUT_MPI
done
