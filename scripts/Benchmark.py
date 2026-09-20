#! /usr/bin/python
import sys
import time
import subprocess
import re
import math

regex_bm_time = re.compile(r"\{(\D*)(\d+)\s+\}", re.MULTILINE)
regex_gc_time = re.compile("\[(\d+(.\d+)?)\]", re.MULTILINE)
Z=1.96

class Benchmark(object):
    def __init__(self, directory, benchmark, iterations):
        self.directory = directory
        self.benchmark = benchmark
        self.iterations = iterations

    def get_name(self):
        return self.benchmark[self.benchmark.rfind("/")+1 : self.benchmark.find(".")]

    def calc_avg_times(self):
        avg = (reduce(lambda x, y: x*y, self.times))**(1.0/len(self.times))
        gc_avg = (reduce(lambda x, y: x*y, self.gc_times))**(1.0/len(self.gc_times))
        std_dev = math.sqrt((sum((x-avg)**2 for x in self.times))/len(self.times))
        conf_int = Z * (std_dev / math.sqrt(self.iterations))
        gc_std_dev = math.sqrt((sum((x-avg)**2 for x in
            self.gc_times))/len(self.gc_times))
        gc_conf_int = Z * (gc_std_dev / math.sqrt(self.iterations))
        return (avg,
                conf_int,
                gc_avg,
                gc_conf_int)

    def run(self):
        try:
            exe = "%s/SOM++ -cp %s/Smalltalk " % (self.directory, self.directory)
            cl = exe + " " + self.benchmark
            print self.get_name(),
            sys.stdout.flush()
            self.times = []
            self.gc_times = []

            for i in xrange(self.iterations):
                start_time=time.time()
                proc = subprocess.Popen(cl.split(), shell=False,
                    bufsize=2048,stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    env={"LD_LIBRARY_PATH" : self.directory},cwd=self.directory)
                data=proc.communicate()[0]
                self.times.append((time.time() - start_time) * 1000)
                gc_res = regex_gc_time.search(data)
                self.gc_times.append(float(gc_res.groups(0)[0]))
                sys.stdout.write(".")
                sys.stdout.flush()
            print ""
        except OSError:
            raise Exception("Error when executing benchmark: " + cl)
        except AttributeError:
            print "LD_LIBRARY_PATH: ", self.directory

            print "Command: ", cl
            raise Exception("Unable to parse benchmark results! Result was:\n"
                    + data)


    def get_csv(self):
        time_strings = [str(self.times[i]) +", "+str(self.gc_times[i]) for i in
                xrange(self.iterations)]
        avgs = self.calc_avg_times()
        s = "%s, %f, %f, %f, %f, " % (self.get_name(), avgs[0], avgs[1],
                avgs[2], avgs[3])
        return s + ", ".join(time_strings)
