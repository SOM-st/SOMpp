set terminal postscript eps enhanced solid color
set output "gc_times_som.eps"
set style data histogram
set style histogram rowstacked
set style fill solid 1.00 border 0 #fill bars
set boxwidth 0.9
set title "Garbage collection times for SOM benchmarks" font "Helvetica,26"
set xlabel  offset character 0, -2, 0 font "" textcolor lt -1 norotate
set ylabel "Average execution time (ms)" font "Helvetica,20"
set xtics nomirror rotate by -45

set grid ytics

set xtics   ("Bounce" 0*7+2, "BubbleSort" 1*7+2, "Dispatch" 2*7+2, "Fibonacci" 3*7+2, "IntegerLoop" 4*7+2,\
             "List" 5*7+2, "Loop" 6*7+2, "Permute" 7*7+2, "Queens" 8*7+2, "QuickSort" 9*7+2,\
	     "Recurse" 10*7+2, "Sieve" 11*7+2, "Storage" 12*7+2, "Sum" 13*7+2, "Towers" 14*7+2,\
	     "TreeSort" 15*7+2)

plot newhistogram lt 1, \
     "benchmark_results/som_benchmarks.csv" index 0 u ($2-$4) title "remaining time", \
     "" index 0 u 4 title "gc time", \
     newhistogram lt 1, \
     "" index 1 u ($2-$4) notitle, \
     "" index 1 u 4 notitle, \
     newhistogram lt 1, \
     "" index 2 u ($2-$4) notitle, \
     "" index 2 u 4 notitle, \
     newhistogram lt 1, \
     "" index 3 u ($2-$4) notitle, \
     "" index 3 u 4 notitle, \
     newhistogram lt 1, \
     "" index 4 u ($2-$4) notitle, \
     "" index 4 u 4 notitle, \
     newhistogram lt 1, \
     "" index 5 u ($2-$4) notitle, \
     "" index 5 u 4 notitle, \
     newhistogram lt 1, \
     "" index 6 u ($2-$4) notitle, \
     "" index 6 u 4 notitle, \
     newhistogram lt 1, \
     "" index 7 u ($2-$4) notitle, \
     "" index 7 u 4 notitle, \
     newhistogram lt 1, \
     "" index 8 u ($2-$4) notitle, \
     "" index 8 u 4 notitle, \
     newhistogram lt 1, \
     "" index 9 u ($2-$4) notitle, \
     "" index 9 u 4 notitle, \
     newhistogram lt 1, \
     "" index 10 u ($2-$4) notitle, \
     "" index 10 u 4 notitle, \
     newhistogram lt 1, \
     "" index 11 u ($2-$4) notitle, \
     "" index 11 u 4 notitle, \
     newhistogram lt 1, \
     "" index 12 u ($2-$4) notitle, \
     "" index 12 u 4 notitle, \
     newhistogram lt 1, \
     "" index 13 u ($2-$4) notitle, \
     "" index 13 u 4 notitle, \
     newhistogram lt 1, \
     "" index 14 u ($2-$4) notitle, \
     "" index 14 u 4 notitle, \
     newhistogram lt 1, \
     "" index 15 u ($2-$4) notitle, \
     "" index 15 u 4 notitle 
