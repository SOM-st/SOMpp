#! /usr/bin/python
from Benchmark import Benchmark

class BenchmarkRunner(object):
    def __init__(self, target_path, no_iterations=10):
        self.no_iterations = no_iterations
        self.target_path = target_path
        self.benchmarks = []
        self.csv = [self.get_csv_header()]

    def add_benchmark(self, bm_path):
        self.benchmarks.append(bm_path)

    def get_csv_header(self):
        header = ["#name", "avg_time", "avg_time_err", "avg_gc_time",
        "avg_gc_time_err"]
        for i in xrange(self.no_iterations):
            header.append("total_time_"+str(i))
            header.append("gc_time_"+str(i))
        return ", ".join(header)

    def get_csv(self):
        return "\n".join(self.csv)

    def run_benchmarks(self):
        print "Executing Benchmarks for " + self.target_path
        for bench_name in self.benchmarks:
            bench = Benchmark(self.target_path, self.target_path+"/"+bench_name,
                    self.no_iterations)
            bench.run()
            self.csv.append(bench.get_csv())
