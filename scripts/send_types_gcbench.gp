reset
set terminal postscript enhanced eps solid color
set output "send_types_gcbench.eps"
set title "Types of sends for GCBench" font "Helvetica,26"
set xtics rotate by -45    #rotate labels
set datafile separator "," #csv is comma separated
set style fill solid 1.00 border 0
set grid ytics
set boxwidth 0.6
set ylabel "Number of sends" font "Helvetiva,20"
plot "benchmark_results/GCBench_send_types.csv" using ($3+$4):xtic(1) ti "primitive" with boxes, \
     ""                                                    using 4:xtic(1) ti "non primitive" with boxes
