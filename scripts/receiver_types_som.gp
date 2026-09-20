set terminal postscript eps enhanced solid color
set output "receiver_types_som.eps"
set title "Receiver types (Fibonacci Benchmarks)"
set xtics rotate by -90    #rotate labels
set datafile separator "," #csv is comma separated
set style fill solid 1.00 border 0 #fill bars
set boxwidth 0.6
set ylabel "Number of sends"
#set logscale y
plot "benchmark_results/Fibonacci_receivers.csv" using 2:xtic(1) ti "number of sends" with boxes
