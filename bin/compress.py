#!/usr/bin/env python

import os, re

def main(args):
    # Compresses ramp tables within the include files given on the
    # commandline

    if len(args) == 0:
        print("""\
Usage: compress.py files ...

If any "#define/#undef PWMn_LEVELS" lines are found, a new PWMn_C_LEVLS line
will be added after it (or updated if the file already contains compressed
tables).  If any changes are found, a new file will be created, and the
original file will be renamed with a ".bak" extension. """)
        return

    print("%d files modified." % (compress_files(args)))
    
    return

def compress_table(oldtable):
    # Given n array of ramp levels, return a string representing the
    # compressed version.  See fsm-ramping.c for an explanation of the
    # algorithm.

    # convert array elements to ints
    table = []
    for i in oldtable:
        table.append(int(round(float(i))))

    # remove leading dupes
    min = table[0]
    min_count = 0

    while table[0] == min:
        min_count = min_count+1
        del table[0]

    # remove turbo
    turbo = table[-1]
    del table[-1]

    # remove trailing dupes
    max = table[-1]

    while table[-1] == max:
        del table[-1]

    return ('%i,%i,%i,%i,%i, %s' % (
        min, min_count, len(table), max, turbo,
        ','.join([str(i) for i in table]) ))


def compress_files(files):
    # loop through each file and add PWMn_C_LEVELS defines as needed

    pwm_re = re.compile("^#(define|undef) PWM([0-9]+)_LEVELS([0-9, \t]*)")
    changed = 0

    for file in files:
        lines = open(file).readlines()
        newlines = []
        modified = 0
        line = 0

        while line < len(lines):
            # copy lines[] to newlines[], adding/replacing rows as needed
            newlines += lines[line]

            # look for PWMn_LEVELS defines
            match = pwm_re.match(lines[line])
            if match:
                # bah. process continuations
                while lines[line].endswith("\\\n"):
                    lines[line] = lines[line][:-2] + lines[line+1]
                    newlines += lines[line+1]
                    del lines[line+1]
                    match = pwm_re.match(lines[line])

                # generate compressed table line
                channel = match.group(2)
                table = match.group(3)
                if match.group(1) == "define":
                    newline = "#define PWM%s_C_LEVELS %s\n" % (
                        channel, compress_table(table.split(",")))
                else:
                    newline = "#undef PWM%s_C_LEVELS\n" % ( channel )

                # If the next line is already a C_LEVELS define
                if line + 1 < len(lines) and \
                    re.match("^#(define|undef) PWM"+channel+"_C_LEVELS", lines[line+1]):
                    # replace it if it's different
                    if newline != lines[line+1]:
                        line = line + 1
                        newlines += newline
                        modified = 1
                else:
                    # otherwise, insert
                    newlines += newline
                    modified = 1
            line = line + 1

        # only replace the file if we had to change it
        if modified:
            os.rename(file, file+".bak")
            open(file, "w").writelines(newlines)
            changed = changed + 1

    return changed


if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
