#!/bin/bash

PROGRAM="secuencial"
SOURCE="sequential.c"
SIZE=1000
OUTDIR="profiling"

mkdir -p $OUTDIR

gcc -o ${PROGRAM} ${SOURCE}
gcc -pg -o ${PROGRAM}_pg ${SOURCE}

perf stat ./${PROGRAM} ${SIZE} 2> ${OUTDIR}/perf_stat.txt
perf stat -d ./${PROGRAM} ${SIZE} 2> ${OUTDIR}/perf_stat_detailed.txt
perf stat -e task-clock,context-switches,cpu-migrations,page-faults,minor-faults,major-faults ./${PROGRAM} ${SIZE} 2> ${OUTDIR}/perf_software_events.txt

perf record -o ${OUTDIR}/perf.data ./${PROGRAM} ${SIZE}
perf report -i ${OUTDIR}/perf.data --stdio > ${OUTDIR}/perf_report.txt

./${PROGRAM}_pg ${SIZE}
mv gmon.out ${OUTDIR}/gmon.out
gprof ${PROGRAM}_pg ${OUTDIR}/gmon.out > ${OUTDIR}/gprof.txt

valgrind --tool=cachegrind ./${PROGRAM} ${SIZE} > ${OUTDIR}/cachegrind_raw.txt 2>&1
cg_annotate cachegrind.out.* > ${OUTDIR}/cachegrind_report.txt

valgrind --tool=massif ./${PROGRAM} ${SIZE} > ${OUTDIR}/massif_raw.txt 2>&1
ms_print massif.out.* > ${OUTDIR}/massif_report.txt
