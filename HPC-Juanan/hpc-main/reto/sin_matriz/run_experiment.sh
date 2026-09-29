#!/bin/bash

gcc -o sequential sequential.c -lm
gcc -o process process.c -lm -lrt
gcc -o threads threads.c -lm -lpthread

ks=(3 4 5 6 7 8 9 10 11 12)
procs=(2 4 6 8)

# --- Secuencial ---
mkdir -p secuencial_time
TIME_CSV="secuencial_time/times.csv"
ITER_CSV="secuencial_time/iterations.csv"

echo -n "Run" > "$TIME_CSV"
echo -n "Run" > "$ITER_CSV"
for k in "${ks[@]}"; do
  n=$(( (1 << k) + 1 ))
  echo -n ",$k/$n" >> "$TIME_CSV"
  echo -n ",$k/$n" >> "$ITER_CSV"
done
echo "" >> "$TIME_CSV"
echo "" >> "$ITER_CSV"

for j in {1..10}; do
  echo -n "$j" >> "$TIME_CSV"
  echo -n "$j" >> "$ITER_CSV"
  echo "Secuencial - Test $j:"
  for k in "${ks[@]}"; do
    output=$(./sequential "$k")
    t=$(echo "$output" | awk '{print $1}')
    iters=$(echo "$output" | awk '{print $2}')
    echo -n ",$t" >> "$TIME_CSV"
    echo -n ",$iters" >> "$ITER_CSV"
    echo "  k=$k done!"
  done
  echo "" >> "$TIME_CSV"
  echo "" >> "$ITER_CSV"
  echo ""
done

# --- Procesos ---
mkdir -p process_time

for p in "${procs[@]}"; do
  TIME_CSV="process_time/times_p${p}.csv"
  ITER_CSV="process_time/iterations_p${p}.csv"

  echo -n "Run" > "$TIME_CSV"
  echo -n "Run" > "$ITER_CSV"
  for k in "${ks[@]}"; do
    n=$(( (1 << k) + 1 ))
    echo -n ",$k/$n" >> "$TIME_CSV"
    echo -n ",$k/$n" >> "$ITER_CSV"
  done
  echo "" >> "$TIME_CSV"
  echo "" >> "$ITER_CSV"

  for j in {1..10}; do
    echo -n "$j" >> "$TIME_CSV"
    echo -n "$j" >> "$ITER_CSV"
    echo "Procesos p=$p - Test $j:"
    for k in "${ks[@]}"; do
      output=$(./process "$k" "$p")
      t=$(echo "$output" | awk '{print $1}')
      iters=$(echo "$output" | awk '{print $2}')
      echo -n ",$t" >> "$TIME_CSV"
      echo -n ",$iters" >> "$ITER_CSV"
      echo "  k=$k done!"
    done
    echo "" >> "$TIME_CSV"
    echo "" >> "$ITER_CSV"
    echo ""
  done
done

# --- Hilos ---
mkdir -p threads_time

for p in "${procs[@]}"; do
  TIME_CSV="threads_time/times_t${p}.csv"
  ITER_CSV="threads_time/iterations_t${p}.csv"

  echo -n "Run" > "$TIME_CSV"
  echo -n "Run" > "$ITER_CSV"
  for k in "${ks[@]}"; do
    n=$(( (1 << k) + 1 ))
    echo -n ",$k/$n" >> "$TIME_CSV"
    echo -n ",$k/$n" >> "$ITER_CSV"
  done
  echo "" >> "$TIME_CSV"
  echo "" >> "$ITER_CSV"

  for j in {1..10}; do
    echo -n "$j" >> "$TIME_CSV"
    echo -n "$j" >> "$ITER_CSV"
    echo "Hilos t=$p - Test $j:"
    for k in "${ks[@]}"; do
      output=$(./threads "$k" "$p")
      t=$(echo "$output" | awk '{print $1}')
      iters=$(echo "$output" | awk '{print $2}')
      echo -n ",$t" >> "$TIME_CSV"
      echo -n ",$iters" >> "$ITER_CSV"
      echo "  k=$k done!"
    done
    echo "" >> "$TIME_CSV"
    echo "" >> "$ITER_CSV"
    echo ""
  done
done