#!/bin/bash
echo "======= System info ======="
echo ""

echo "== CPU =="
lscpu
echo ""

echo "== Free memory =="
free -h
echo ""

echo "== Operating system =="
uname -a
echo ""
