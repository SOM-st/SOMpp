set terminal postscript eps enhanced solid color
set output "som_benchmarks_with_cache.eps"
set title "SOM Benchmarks with integer caching" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:]            #plot starting from 0
set xtics rotate by -45    #rotate labels
set ylabel "Average execution time (ms)" font "Helvetica,20"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
set grid ytics
plot "benchmark_results/generational_cache_noTagging_som.csv" using 2:3:xtic(1) ti "SOM++ generational", \
     "benchmark_results/generational_cache_tagging_som.csv" using 2:3 ti "SOM++ generational (tagging)", \
     "benchmark_results/copying_cache_noTagging_som.csv" using 2:3 ti "SOM++ copying", \
     "benchmark_results/copying_cache_tagging_som.csv" using 2:3 ti "SOM++ copying (tagging)", \
     "benchmark_results/mark_sweep_cache_noTagging_som.csv" using 2:3 ti "SOM++ mark-sweep", \
     "benchmark_results/mark_sweep_cache_tagging_som.csv" using 2:3 ti "SOM++ mark-sweep (tagging)"
