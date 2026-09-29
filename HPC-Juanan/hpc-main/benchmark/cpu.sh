#!/bin/bash
echo "======= CPU ======="
echo ""

echo "== Testing with 1 thread  =="
sysbench cpu --cpu-max-prime=20000 --threads=1 --time=60 run
echo ""

echo "== Testing with all available threads =="
sysbench cpu --cpu-max-prime=20000 --threads=$(nproc) --time=60 run
echo ""
