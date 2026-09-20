#!/usr/bin/python
from os import listdir
from os import rename
from subprocess import call
import argparse
import shutil

OUT_DIR = "generated_images/"

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='Run Benchmarks for all SOM++ VMs')
    parser.add_argument("output_types", nargs="*", default = ["eps"])
    args = parser.parse_args()

    files=[s for s in listdir("benchmark_results") if
            s.endswith("allocation_statistics.csv")]
    for csv_file in files:
        name = csv_file[:csv_file.find("_")]
        csv_file = "benchmark_results/"+csv_file

        #generate no plots
        out_file = "allocation_statistics_no_" + name + ".eps"
        command = "sed 's/bench_name/%s/g;s/csv_file/%s/g;s/out_file/%s/g' < scripts/allocation_statistics_no_template.gpl | gnuplot" % (name, csv_file.replace("/","\/"), out_file)
        print "Executing: %s" % command
        call(command, shell=True)
        eps_file = OUT_DIR + out_file
        shutil.move(out_file, eps_file)
        if "png" in args.output_types:
            png_file = eps_file.replace(".eps", ".png")
            call(["convert", "-density", "300", eps_file, png_file])
        if "pdf" in args.output_types:
            print "epstopdf %s" % eps_file
            call(["epstopdf %s" % eps_file], shell=True)

        # also generate size plots
        out_file = "allocation_statistics_size_" + name + ".eps"
        command = "sed 's/bench_name/%s/g;s/csv_file/%s/g;s/out_file/%s/g' < scripts/allocation_statistics_size_template.gpl | gnuplot" % (name, csv_file.replace("/","\/"), out_file)
        print "Executing: %s" % command
        call(command, shell=True)
        eps_file = OUT_DIR + out_file
        shutil.move(out_file, eps_file)
        if "png" in args.output_types:
            png_file = eps_file.replace(".eps", ".png")
            call(["convert", "-density", "300", eps_file, png_file])
        if "pdf" in args.output_types:
            print "epstopdf %s" % eps_file
            call(["epstopdf %s" % eps_file], shell=True)
