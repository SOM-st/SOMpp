set terminal postscript eps enhanced solid color
set output "gcbench_no_cache.eps"
set title "GCBench Benchmark" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:]            #plot starting from 0
set key left
set ylabel "Average execution time (s)" font "Helvetica,20"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
set grid ytics
plot "benchmark_results/generational_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000):xtic(1) ti "SOM++ generational", \
     "benchmark_results/generational_nocache_tagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ generational (tagging)", \
     "benchmark_results/copying_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ copying", \
     "benchmark_results/copying_nocache_tagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ copying (tagging)", \
     "benchmark_results/mark_sweep_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ mark-sweep", \
     "benchmark_results/mark_sweep_nocache_tagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ mark-sweep (tagging)"
