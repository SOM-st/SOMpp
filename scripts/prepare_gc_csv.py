import os
import sys

#! /usr/bin/python
base_path = os.path.abspath(os.path.dirname(sys.argv[0]) + "/../")

#
# merges columns from several csv files to one csv file
#   the first column is always kept
#
def merge(columns, files):
    #first read all csv files to an 3-dim array raw-csv[file][line][column]
    raw_csv = []
    for f in files:
        csv_arr=[]
        for line in file(f).readlines():
            csv_arr.append([elem.strip() for elem in line.split(",")])
        raw_csv.append(csv_arr)

    #now create new file
    result = []
    for bench_index in xrange(len(raw_csv[0])):
        for f in xrange(len(files)):
            result.append(", ".join(raw_csv[f][bench_index]))
        result.extend(["",""])

    return "\n".join(result)

if __name__ == "__main__":
    #SOM Benchmarks
    # - comparation of non-tagging vs tagging version (without int cache)
    csv = merge((1,2,3,4),
            [   base_path+"/benchmark_results/generational_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/generational_nocache_tagging_som.csv",
                base_path+"/benchmark_results/copying_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/copying_nocache_tagging_som.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_noTagging_som.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_tagging_som.csv"
                ])
    f = file(base_path+"/benchmark_results/som_benchmarks.csv", "w")
    f.write(csv)
    f.close()

    #Richards Benchmark
    # - comparation of non-tagging vs tagging version (without int cache)
    csv = merge((1,2,3,4),
            [   base_path+"/benchmark_results/generational_nocache_noTagging_richards.csv",
                base_path+"/benchmark_results/generational_nocache_tagging_richards.csv",
                base_path+"/benchmark_results/copying_nocache_noTagging_richards.csv",
                base_path+"/benchmark_results/copying_nocache_tagging_richards.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_noTagging_richards.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_tagging_richards.csv"
                ])
    f = file(base_path+"/benchmark_results/richards_benchmark.csv", "w")
    f.write(csv)
    f.close()


    #GCBench
    # - comparation of non-tagging vs tagging version (without int cache)
    csv = merge((1,2,3,4),
            [
                base_path+"/benchmark_results/generational_nocache_noTagging_gcbench.csv",
                base_path+"/benchmark_results/generational_nocache_tagging_gcbench.csv",
                base_path+"/benchmark_results/copying_nocache_noTagging_gcbench.csv",
                base_path+"/benchmark_results/copying_nocache_tagging_gcbench.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_noTagging_gcbench.csv",
                base_path+"/benchmark_results/mark_sweep_nocache_tagging_gcbench.csv"
                ])
    f = file(base_path+"/benchmark_results/gcbench.csv", "w")
    f.write(csv)
    f.close()




