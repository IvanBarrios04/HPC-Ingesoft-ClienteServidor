#!/bin/bash
echo "======= Memory ======="
echo ""

echo "== Input =="
sysbench memory --memory-block-size=1M --memory-total-size=10G --memory-oper=read run
echo ""

echo "== Output =="
sysbench memory --memory-block-size=1M --memory-total-size=10G --memory-oper=write run
echo ""
