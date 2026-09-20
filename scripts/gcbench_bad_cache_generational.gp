set terminal postscript eps enhanced solid color
set output "gcbench_bad_cache_generational.eps"
set title "GCBench - caching" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:120]      #plot starting from 0
set ylabel "Average execution time (s)" font "Helvetica,20"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
set key left
set grid ytics
plot "benchmark_results/generational_nocache_noTagging_gcbench.csv" using ($2/1000):($3/1000):xtic(1) ti "SOM++ generational (no cache)", \
     "benchmark_results/generational_badcache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ generational (bad cache)", \
     "benchmark_results/generational_cache_noTagging_gcbench.csv" using ($2/1000):($3/1000) ti "SOM++ generational (good cache)"
