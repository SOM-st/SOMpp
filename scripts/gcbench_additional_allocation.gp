set terminal postscript eps enhanced solid color
set output "gcbench_additional_allocation.eps"
set title "GCBench Benchmark - additional allocation" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:350]            #plot starting from 0
set key left
set ylabel "Average execution time (s)" font "Helvetica,20"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
set grid ytics
plot "benchmark_results/generational_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000):xtic(1) ti "GCBench generational", \
     "benchmark_results/generational_additional_allocation_gcbench.csv" using ($2/1000):($3/1000) ti "GCBench generational (tagging+allocation)", \
     "benchmark_results/copying_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "GCBench copying", \
     "benchmark_results/copying_additional_allocation_gcbench.csv" using ($2/1000):($3/1000) ti "GCBench copying (tagging+allocation)", \
     "benchmark_results/mark_sweep_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "GCBench mark-sweep", \
     "benchmark_results/mark_sweep_additional_allocation_gcbench.csv" using ($2/1000):($3/1000) ti "GCBench mark-sweep (tagging+allocation)"
