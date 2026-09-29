#!/bin/bash
OUTPUT="results-$(hostname)-$(date +%Y%m%d-%H%M%S).txt"

echo "Saving results in $OUTPUT"

bash system-info.sh >> "$OUTPUT" 2>&1
bash cpu.sh         >> "$OUTPUT" 2>&1
bash memory.sh      >> "$OUTPUT" 2>&1

echo "Ready. Results in $OUTPUT"
