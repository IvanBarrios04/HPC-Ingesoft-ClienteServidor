#!/bin/bash

sizes=(10000 100000 1000000 10000000 100000000 1000000000)
DENSITY=0.5
SEED=31
OUTPUT_SEQ="times_sequential.csv"
OUTPUT_OMP="times_openmp.csv"

mkdir -p bin

gcc sequential.c -o ./bin/sequential || { echo "Error compiling sequential"; exit 1; }
gcc openmp.c -o ./bin/openmp -fopenmp || { echo "Error compiling openmp"; exit 1; }

# Sequential CSV
echo -n "run" > $OUTPUT_SEQ
for s in "${sizes[@]}"; do
    echo -n ",$s" >> $OUTPUT_SEQ
done
echo "" >> $OUTPUT_SEQ

# OpenMP CSV
echo -n "run" > $OUTPUT_OMP
for s in "${sizes[@]}"; do
    echo -n ",$s" >> $OUTPUT_OMP
done
echo "" >> $OUTPUT_OMP

# Sequential runs
for j in {1..10}; do
    echo "Sequential Run $j..."
    echo -n "$j" >> $OUTPUT_SEQ
    for s in "${sizes[@]}"; do
        t_seq=$(./bin/sequential $s $DENSITY $SEED)
        echo -n ",$t_seq" >> $OUTPUT_SEQ
    done
    echo "" >> $OUTPUT_SEQ
done

# OpenMP runs
for j in {1..10}; do
    echo "OpenMP Run $j..."
    echo -n "$j" >> $OUTPUT_OMP
    for s in "${sizes[@]}"; do
        t_omp=$(./bin/openmp $s $DENSITY $SEED)
        echo -n ",$t_omp" >> $OUTPUT_OMP
    done
    echo "" >> $OUTPUT_OMP
done

echo "Ready. Results in $OUTPUT_SEQ and $OUTPUT_OMP"
