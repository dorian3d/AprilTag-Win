#!/usr/bin/env python
import sys, os
import re
from collections import defaultdict
from operator import itemgetter

# L is the measurement length
# PL is the total path length
def scan_tests(folder_name):
    # Set up the tests by scanning the sequences folder
    configurations = []
    for dirname, dirnames, filenames in os.walk(folder_name, followlinks=True):
        for filename in filenames:
            (ignore, config_name) = os.path.split(dirname.rstrip('/'))
            L_match = re.search("_L([\d.]+)", filename)
            PL_match = re.search("_PL([\d.]+)", filename)
            L = None
            PL = None
            if L_match: 
                L = float(L_match.group(1))
            if PL_match: 
                PL = float(PL_match.group(1))
            if not L_match and not PL_match:
                print "Malformed data filename:", filename, "skipping"
                continue
            
            test_case = {"config" : config_name, "path" : os.path.join(dirname, filename), "L" : L, "PL" : PL}
            configurations.append(test_case)
    return sorted(configurations, key=itemgetter('config', 'path'))

def measurement_error(L, L_measured):
    err = abs(L_measured - L)
    if L == 0:
        return (err, err)
    return (err, 100.*err / L)

def measurement_string(L, L_measured):
    error, error_percent = measurement_error(L, L_measured)
    return "%.2fcm actual, %.2fcm measured, %.2fcm error (%.2f%%)" % (L, L_measured, error, error_percent)

def run_test_case(test_case):
    print "Running", test_case["path"]; sys.stdout.flush();
    return measure(test_case["path"], test_case["config"])

import subprocess
def subprocess_test_case(test_case):
    print "Running", test_case["path"], "using bin/measure"; sys.stdout.flush();
    output = subprocess.check_output("../corvis/bin/measure %s %s" %
            (test_case["path"], test_case["config"]), stderr=subprocess.STDOUT, shell=True)
    #"Straight-line length is 89.00 cm, total path length 92.54 cm"
    res = re.match(".* ([\d\.]+) cm.* ([\d\.]+) cm.*",
            output, re.MULTILINE | re.DOTALL)
    (PL, L) = (None, None)
    if res:
        L = float(res.group(1))
        PL = float(res.group(2))
    print "Finished", test_case["path"], "(%.2fcm, %.2fcm)" % (L, PL); sys.stdout.flush();
    return (PL, L)

import numpy
def error_histogram(errors, _bins = [0, 3, 10, 25, 50, 100]):
    bins = list(_bins)
    if len(errors) > 0 and max(errors) > 100:
        bins.append(max(errors))
    (counts, bins) = numpy.histogram(errors, bins)
    return (counts, bins)

def error_histogram_string(counts, bins):
    hist_str = ""
    bins_str = []
    for b in bins:
        bins_str.append("%.1f%%" % b)
    bins_str = bins_str[:-1]
    bins_str[-1] += "+"
    hist_str += "\t".join(bins_str) + "\n"
    counts = [str(count) for count in counts]
    hist_str += "\t".join(counts) + "\n"
    return hist_str

import multiprocessing
from measure import measure

def benchmark(folder_name):
    test_cases = scan_tests(folder_name)
    pool = multiprocessing.Pool(multiprocessing.cpu_count() - 1)
    print "Worker pool size is", pool._processes
    #results = pool.map(run_test_case, test_cases)
    results = pool.map(subprocess_test_case, test_cases)

    L_errors_percent = []
    PL_errors_percent = []
    primary_errors_percent = []
    for test_case, result in zip(test_cases, results):
        (PL, L) = result
        print "Result", test_case["path"]
        has_L = test_case["L"] is not None
        has_PL = test_case["PL"] is not None
        # Length measurement
        base_L = test_case["L"] if has_L else 0.;
        base_PL = test_case["PL"] if has_PL else PL;
        if base_PL == 0:
            base_PL = 1.;
        
        L_error, L_error_percent = measurement_error(base_L, L)
        (PL_error, PL_error_percent) = measurement_error(base_PL, PL)
        loop_close_error_percent = 100. * abs(L - base_L) / base_PL;

        if has_L:
            print "\tL\t%s" % measurement_string(base_L, L)
            L_errors_percent.append(L_error_percent)
            if has_PL or (PL > 5 and base_L <= 5):
                #either explicitly closed the loop, or an implicit loop closure using measured PL as loop length
                print "\t", "Loop closure error: %.2f%%" % loop_close_error_percent
                primary_errors_percent.append(loop_close_error_percent)
            elif base_L > 5:
                primary_errors_percent.append(L_error_percent)
            else:
                primary_errors_percent.append(L_error)
        else:
            primary_errors_percent.append(PL_error_percent)

        if has_PL:
            print "\tPL\t%s" % measurement_string(base_PL, PL)
            PL_errors_percent.append(PL_error_percent)

    (counts, bins) = error_histogram(L_errors_percent)
    print "Length error histogram (%d sequences)" % len(L_errors_percent)
    print error_histogram_string(counts, bins)

    (counts, bins) = error_histogram(PL_errors_percent)
    print "Path length error histogram (%d sequences)" % len(PL_errors_percent)
    print error_histogram_string(counts, bins)

    (counts, bins) = error_histogram(primary_errors_percent)
    print "Primary error histogram (%d sequences)" % len(primary_errors_percent)
    print error_histogram_string(counts, bins)

    (altcounts, altbins) = error_histogram(primary_errors_percent, [0, 4, 12, 30, 65, 100])
    print "Alternate error histogram (%d sequences)" % len(primary_errors_percent)
    print error_histogram_string(altcounts, altbins)

    pe = numpy.array(primary_errors_percent)
    ave_error = pe[pe < 50.].mean()

    print "Mean of %d primary errors that are less than 50%% is %.2f%%" % (sum(pe<50), ave_error)

    score = 0
    for i in range(0, counts.size):
        score = score + i * counts[i]

    altscore = 0
    for i in range(0, altcounts.size):
        altscore = altscore + i * altcounts[i]

    print "Histogram score (lower is better) is %d" % score
    print "Alternate histogram score (lower is better) is %d\n" % altscore

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage:", sys.argv[0], "<sequence folder>"
        sys.exit(1)

    benchmark(sys.argv[1])





