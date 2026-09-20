set terminal postscript eps enhanced solid color
set output "som_benchmarks_bad_cache_mark.eps"
set title "SOM Benchmarks - caching" font "Helvetica,26"
set datafile separator "," #csv is comma separated
set yrange [0:]      #plot starting from 0
set xtics rotate by -45    #rotate labels
set ylabel "Average execution time (ms)" font "Helvetica,20"
set style data histograms  #plot histogram style
set style fill solid 1.00 border 0 #fill bars
set style histogram errorbars gap 2 lw 1
set key right
set grid ytics
plot "benchmark_results/mark_sweep_nocache_noTagging_som.csv" using 2:3:xtic(1) ti "SOM++ mark-sweep (no cache)", \
     "benchmark_results/mark_sweep_badcache_noTagging_som.csv" using 2:3 ti "SOM++ mark-sweep (bad cache)", \
     "benchmark_results/mark_sweep_cache_noTagging_som.csv" using 2:3 ti "SOM++ mark-sweep (good cache)"
