#!/usr/bin/env python
import sys, os, errno
import re, math
import subprocess
from collections import defaultdict
from operator import itemgetter

# L is the measurement length
# PL is the total path length
def scan_tests(folder_name):
    if folder_name[-1] != '/': folder_name += '/' # Allows stripping below
    # Set up the tests by scanning the sequences folder
    configurations = []
    for dirname, dirnames, filenames in os.walk(folder_name, followlinks=True):
        for filename in filenames:
            config_name = os.path.basename(os.path.dirname(os.path.join(dirname, filename)))
            if filename.endswith(".json") or filename.endswith(".pose"):
                continue;
            test_case = {"config" : config_name, "path" : os.path.join(dirname[len(folder_name):],filename) }
            configurations.append(test_case)
    return sorted(configurations, key=itemgetter('config', 'path'))

def measurement_error(L, L_measured):
    err = abs(L_measured - L)
    if L < 1:
        return (err, err)
    return (err, 100.*err / L)

def measurement_string(L, L_measured):
    error, error_percent = measurement_error(L, L_measured)
    return "%.2fcm actual, %.2fcm measured, %.2fcm error (%.2f%%)" % (L, L_measured, error, error_percent)

class TestRunner(object):
  def  __init__(self, input_dir, output_dir = None, qvga = False):
    self.input_dir = input_dir
    self.output_dir = output_dir
    self.measure_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../bin/measure")
    self.qvga = qvga

  def __call__(self, test_case):
    #return self.run(test_case)
    return self.run_subprocess(test_case)

  def run_subprocess(self, test_case):
    print "Running ", test_case["path"]; sys.stdout.flush();
    args = [self.measure_path, os.path.join(self.input_dir, test_case["path"]), test_case["config"], "--no-gui"]
    if self.qvga:
        args.append("--qvga")
    if self.output_dir is not None:
        test_case["image"] = "%s.png" % test_case["path"]
        image = os.path.join(output_dir, test_case["image"])
        try:    os.makedirs(os.path.dirname(image))
        except OSError as e:
            if e.errno != errno.EEXIST: raise
        args.extend(["--render", image])
    output = subprocess.check_output(args, stderr=subprocess.STDOUT)
    print output
    #"Reference Straight-line length is 0.00 cm, total path length 65.00 cm"
    #"Computed  Straight-line length is 0.40 cm, total path length 49.29 cm"
    res = re.match(".*Reference .* ([\d\.]+|nan) cm.* ([\d\.]+|nan) cm.*"
                   +".*Computed .* ([\d\.]+) cm.* ([\d\.]+) cm.*",
                   output, re.MULTILINE | re.DOTALL)
    (L, PL, base_L, base_PL) = map(float,res.group(3,4,1,2)) if res else (None, None, float("nan"), float("nan"))
    print "Finished", test_case["path"], "(%.2fcm, %.2fcm)" % (L, PL) if res else "failed to match"; sys.stdout.flush();
    return (PL, L, base_PL, base_L)

def write_html(output_dir, test_cases):
    with open(os.path.join(output_dir, "index.html"),'w') as html:
        html.write("""<!doctype html><html><head><style>
                      body { color: #fff; background: #000; }
                      </style></head><body>%s</body></html> """ %
                   "\n".join(map(lambda test_case:
                                 "<img src='%s.png' onclick='this.scrollIntoView(true)'><figcaption>%s</figcaption>" %
                                 (test_case["path"], test_case["path"]), test_cases)))

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

def benchmark(input_dir, output_dir = None, qvga = False):
    test_runner = TestRunner(input_dir, output_dir, qvga)
    test_cases = scan_tests(input_dir)
    pool = multiprocessing.Pool(multiprocessing.cpu_count() - 1)
    print "Worker pool size is", pool._processes

    if output_dir:
        write_html(output_dir, test_cases) # do first so you can reload to get progress
        r = open(os.path.join(output_dir, "results.txt"),"w")
    else:
        r = sys.stdout

    results = pool.map(test_runner, test_cases)

    L_errors_percent = []
    PL_errors_percent = []
    primary_errors_percent = []
    for test_case, result in zip(test_cases, results):
        (PL, L, base_PL, base_L) = result
        print >>r, "Result", test_case["path"]

        has_PL = not math.isnan(base_PL)
        has_L = not math.isnan(base_L)
        if not has_L:
            base_L = 0.
        if not has_PL:
            base_PL = PL
        if base_PL == 0:
            base_PL = 1.;

        L_error, L_error_percent = measurement_error(base_L, L)
        (PL_error, PL_error_percent) = measurement_error(base_PL, PL)
        loop_close_error_percent = 100. * abs(L - base_L) / base_PL;

        if has_L:
            print >>r, "\tL\t%s" % measurement_string(base_L, L)
            L_errors_percent.append(L_error_percent)
            if has_PL or (PL > 5 and base_L <= 5):
                #either explicitly closed the loop, or an implicit loop closure using measured PL as loop length
                print >>r, "\t", "Loop closure error: %.2f%%" % loop_close_error_percent
                primary_errors_percent.append(loop_close_error_percent)
            elif base_L > 5:
                primary_errors_percent.append(L_error_percent)
            else:
                primary_errors_percent.append(L_error)
        else:
            primary_errors_percent.append(PL_error_percent)

        if has_PL:
            print >>r, "\tPL\t%s" % measurement_string(base_PL, PL)
            PL_errors_percent.append(PL_error_percent)

    (counts, bins) = error_histogram(L_errors_percent)
    print >>r, "Length error histogram (%d sequences)" % len(L_errors_percent)
    print >>r, error_histogram_string(counts, bins)

    (counts, bins) = error_histogram(PL_errors_percent)
    print >>r, "Path length error histogram (%d sequences)" % len(PL_errors_percent)
    print >>r, error_histogram_string(counts, bins)

    (counts, bins) = error_histogram(primary_errors_percent)
    print >>r, "Primary error histogram (%d sequences)" % len(primary_errors_percent)
    print >>r, error_histogram_string(counts, bins)

    (altcounts, altbins) = error_histogram(primary_errors_percent, [0, 4, 12, 30, 65, 100])
    print >>r, "Alternate error histogram (%d sequences)" % len(primary_errors_percent)
    print >>r, error_histogram_string(altcounts, altbins)

    pe = numpy.array(primary_errors_percent)
    ave_error = pe[pe < 50.].mean()

    print >>r, "Mean of %d primary errors that are less than 50%% is %.2f%%" % (sum(pe<50), ave_error)

    score = 0
    for i in range(0, counts.size):
        score = score + i * counts[i]

    altscore = 0
    for i in range(0, altcounts.size):
        altscore = altscore + i * altcounts[i]

    print >>r, "Histogram score (lower is better) is %d" % score
    print >>r, "Alternate histogram score (lower is better) is %d\n" % altscore

if __name__ == "__main__":
    from optparse import OptionParser
    usage = "Usage: %prog [options] sequence-dir [output-dir]"
    parser = OptionParser(usage=usage)
    parser.add_option("-f", "--force", action="store_true", dest="force", default=False,
            help="Force overriting output-dir")
    parser.add_option("-q", "--qvga", action="store_true", dest="qvga", default=False,
            help="Scale images to qvga (320x240)")
    (options, args) = parser.parse_args()
    if   len(args) == 1:
        sequence_dir, output_dir = args[0], None
    elif len(args) == 2:
        sequence_dir, output_dir = args[0], args[1]
        if not os.path.exists(output_dir) or not options.force:
            os.mkdir(output_dir)
    else:
        parser.print_help()
        sys.exit(1)

    benchmark(sequence_dir, output_dir, options.qvga)


