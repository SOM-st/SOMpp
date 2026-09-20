reset
set terminal postscript enhanced eps solid color
set output "integer_histogram_gcbench.eps"
set title "Integer histogram for GCBench" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:]      #plot starting from 0
set ylabel "Number of integers created" font "Helvetiva,20"
set xlabel "Integer value" font "Helvetiva,20"
set logscale x
set logscale y
set yrange [1:]
plot "benchmark_results/GCBench_integer_histogram.csv" using ($1 < 1 ? 1/0 : $1):2 ti "positive" with points, \
     "" using (-$1):2 ti "negative" with points
