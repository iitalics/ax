#!/usr/bin/bash

ts=$(date '+%m%d_%H%M%S%N')

logfile=_bench/${ts}.txt
echo BENCH - Logging stdout to "${logfile}"

LD_LIBRARY_PATH=_build/lib ./ax_bench $* > ${logfile} &
pid=$!
echo BENCH - PID: ${pid}

data=_bench/${ts}.perf
perf record -p ${pid} -F 999 -a -g
test -f perf.data || (echo "no perf.data file!"; exit 1)
mv perf.data ${data}
echo BENCH - perf data written to ${data}

wait ${pid}

echo BENCH - generating flamegraph
folded=_bench/${ts}.perf-folded
svg=_bench/${ts}.svg
perf script -i ${data} | stackcollapse-perf.pl > ${folded}
flamegraph.pl ${folded} > ${svg}

echo BENCH - generated ${svg}
firefox ${svg}
