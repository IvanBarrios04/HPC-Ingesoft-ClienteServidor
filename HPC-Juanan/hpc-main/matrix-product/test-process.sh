#!/bin/bash

sizes=(400 600 800 1200 1600 2400 3200 4800 6400 9600)
num_process=(2 4 6 8)

for process in "${num_process[@]}"; do
  OUTPUT="times_pro_$process.csv"
  # Write header row
  echo -n "Run" > $OUTPUT
  for s in "${sizes[@]}"; do
    echo -n ",$s" >> $OUTPUT
  done

  echo "" >> $OUTPUT
  
  echo "Tes with $process process: "
  # Runs
  for j in {1..10}; do
    echo -n "$j" >> $OUTPUT
    
    echo "Test $j with each size:"
    for s in "${sizes[@]}"; do
      # time=$(./exe/main.exe $s)
      time=$(./process $s $process)
      echo -n ",$time" >> $OUTPUT
      echo "$s x $s done!"
    done
      
    echo "" >> $OUTPUT
    echo ""
  done
done 
