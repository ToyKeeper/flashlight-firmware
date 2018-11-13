#!/usr/bin/python

from statistics import mode
import re

""" compress.py: Input a cfg-*-h file and get the compressed version
    (with uncompressed lines commented out)

    Example for compression

                         from = 3                to = 9
                         v                       v
    old tbl: 1   2   3   255 255 255 255 255 255 0

    old idx  0   1   2   3   4   5   6   7   8   9 <- this is passed as lvl
    new idx  0   1   2   r   r   r   r   r   r   4
    real     i   i   i   r   r   r   r   r   r   i-(to-from)

    compressed table
             1   2   3                           0

                            F  T  R
    #define PWMx_COMPRESS   3, 9, 255
    #define PWMx_LEVELS     1,2,3,0

"""

def main(args):
    fn = args[0]
    #print("Old ramp:", ramp)

    lines = []

    with open(fn) as f:
        for line in f.readlines():
            lines.append(line)

    with open(fn, "w") as f:
        for line in lines:
            match = re.search("^#undef PWM(\d)_LEVELS", line)
            if match is not None:
                idx = match.group(1)
                f.write("#undef PWM{}_COMPRESS\n".format(idx))
                f.write("#undef PWM{}_LEVELS\n".format(idx))


            match = re.search("^#define PWM(\d)_LEVELS (.*)$", line)
            if match is None:
                f.write(line)
            else:
                f.write("//" + line)

                idx, ramp = match.groups()
                print("Ramp", idx)
                start, end, most_used, ramp = compressRamp(ramp)

                f.write("#define PWM{}_COMPRESS  {}, {}, {}\n".format(idx, start, end, most_used))
                f.write("#define PWM{}_LEVELS    {}\n".format(idx, ",".join(ramp)))


def compressRamp(ramp):
    ramp = ramp.split(",")

    most_used = mode(ramp)

    start = None
    end = None

    for idx, value in enumerate(ramp):
        if value != most_used and start is None:
            None
        elif value == most_used and start is None:
            start = idx
        elif value == most_used and start is not None:
            None
        elif value != most_used and end is None:
            end = idx

    if end is None:
        end = len(ramp)

    ramp = ramp[:start] + ramp[end:]


    print("Most used level:", most_used)
    print("#define PWMx_COMPRESS    {}, {}, {}".format(start, end, most_used))
    print("#define PWMx_LEVELS", ",".join(ramp))
    print("")

    return (start, end, most_used, ramp)



if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
