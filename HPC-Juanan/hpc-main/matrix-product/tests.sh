#!/bin/bash

OUTPUT="times.csv"

sizes=(400 600 800 1200 1600 2400 3200 4800 6400 9600)

# exe="./exe/sequential.exe"
exe="./sequential"

# Write header row
echo -n "Run" > $OUTPUT
for s in "${sizes[@]}"; do
  echo -n ",$s" >> $OUTPUT
done

echo "" >> $OUTPUT

# Runs
for j in {1..10}; do
  echo -n "$j" >> $OUTPUT
  
  echo "Test $j with each size:"
  for s in "${sizes[@]}"; do
    time=$($exe $s)
    echo -n ",$time" >> $OUTPUT
    echo "$s x $s done!"
  done
    
  echo "" >> $OUTPUT
  echo ""
done
