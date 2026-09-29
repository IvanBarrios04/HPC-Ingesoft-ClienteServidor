#!/bin/bash

PROGRAM="secuencial"
SOURCE="../sequential.c"
DENSITY=0.5
SEED=31
SIZE=10000000
OUTDIR="profiling_data"

mkdir -p $OUTDIR

gcc -o ${PROGRAM} ${SOURCE}
gcc -pg -o ${PROGRAM}_pg ${SOURCE}

perf stat ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED} 2> ${OUTDIR}/perf_stat.txt
perf stat -d ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED} 2> ${OUTDIR}/perf_stat_detailed.txt
perf stat -e task-clock,context-switches,cpu-migrations,page-faults,minor-faults,major-faults ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED} 2> ${OUTDIR}/perf_stat_custom.txt

perf record -o ${OUTDIR}/perf.data ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED}
perf report -i ${OUTDIR}/perf.data --stdio > ${OUTDIR}/perf_report.txt

./${PROGRAM}_pg ${SIZE} ${DENSITY} ${SEED}
mv gmon.out ${OUTDIR}/gmon.out
gprof ${PROGRAM}_pg ${OUTDIR}/gmon.out > ${OUTDIR}/gprof.txt

valgrind --tool=cachegrind ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED} > ${OUTDIR}/cachegrind_raw.txt 2>&1
cg_annotate cachegrind.out.* > ${OUTDIR}/cachegrind_report.txt

valgrind --tool=massif ./${PROGRAM} ${SIZE} ${DENSITY} ${SEED} > ${OUTDIR}/massif_raw.txt 2>&1
ms_print massif.out.* > ${OUTDIR}/massif_report.txt
