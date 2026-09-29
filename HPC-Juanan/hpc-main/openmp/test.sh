#!/bin/bash

sizes=(400 600 800 1200 1600 2400 3200 4800 6400 9600)
OUTPUT_OPENMP="times_openmp.csv"
OUTPUT_SEQUENTIAL="times_sequential.csv"

mkdir -p bin

gcc -fopenmp -o bin/openmp openmp.c
gcc -o bin/sequential sequential.c

echo -n "Run" > $OUTPUT_OPENMP
for s in "${sizes[@]}"; do echo -n ",$s" >> $OUTPUT_OPENMP; done
echo "" >> $OUTPUT_OPENMP

for j in {1..10}; do
echo -n "$j" >> $OUTPUT_OPENMP
for s in "${sizes[@]}"; do
time_openmp=$(./bin/openmp $s)
echo -n ",$time_openmp" >> $OUTPUT_OPENMP
done
echo "" >> $OUTPUT_OPENMP
done

echo -n "Run" > $OUTPUT_SEQUENTIAL
for s in "${sizes[@]}"; do echo -n ",$s" >> $OUTPUT_SEQUENTIAL; done
echo "" >> $OUTPUT_SEQUENTIAL

for j in {1..10}; do
echo -n "$j" >> $OUTPUT_SEQUENTIAL
for s in "${sizes[@]}"; do
time_sequential=$(./bin/sequential $s)
echo -n ",$time_sequential" >> $OUTPUT_SEQUENTIAL
done
echo "" >> $OUTPUT_SEQUENTIAL
done
