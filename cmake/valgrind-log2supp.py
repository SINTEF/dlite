#!/usr/bin/env python
"""Converts valgrind log output to a suppressions file.

Run valgrind with something like:

    valgrind --leak-check=full --show-reachable=yes
       --error-limit=no --gen-suppressions=all --num-callers=12
       --log-file=output.log executable_to_check

Use the created `output.log` file as input to this script.

Duplicated suppressions are discarted.
"""
import argparse
import hashlib
import os
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "infile",
        type=argparse.FileType("rt"),
        help="Input file created with the valgrind --log-file option.",
    )
    parser.add_argument(
        "outfile", type=argparse.FileType("wt"), help="Output suppressions file."
    )
    parser.add_argument(
        "--head",
        "-H",
        type=argparse.FileType("rt"),
        help="A file who's content will included first in the output.",
    )
    parser.add_argument(
        "--tail",
        "-t",
        type=argparse.FileType("rt"),
        help="A file who's content will appended to the output.",
    )
    parser.add_argument(
        "--skip-leak-kinds",
        "-s",
        action="store_true",
        help="Whether to skip matching leak kinds.",
    )
    parser.add_argument(
        "--keep-obj",
        "-k",
        action="store_true",
        help=(
            "Whether to keep `obj:<library>` in output.  The default is "
            "to replace it with `obj:*`."
        ),
    )
    parser.add_argument(
        "--exclude-regex",
        "-x",
        metavar="REGEX",
        help="Exclude suppressions matching regular expression REGEX",
    )
    args = parser.parse_args()

    d = parse_input(args)
    write_output(args, d)

    args.infile.close()
    args.outfile.close()


def parse_input(args):
    """Parse the valgrind log file and return a dict mapping md5 hashes to
    suppression sections."""
    d = {}
    n = 1
    sec = []
    fin = args.infile

    for line in fin:
        if line.startswith("=="):
            pass

        elif re.match(r"\s*{", line):
            sec = ["{"]

        elif re.search(r"<[a-z_]+>", line):
            sec.append("   suppression-%d-%s" % (n, os.path.basename(fin.name)))

        elif args.skip_leak_kinds and re.search(r"match-leak-kinds:", line):
            pass

        elif not args.keep_obj and re.search(r"obj:", line):
            # if 'obj:' in sec[-1]:
            #    sec[-1] = '   ...'
            # elif '...' in sec[-1]:
            #    pass
            # else:
            #    sec.append('   obj:*')
            if "..." not in sec[-1]:
                sec.append("   ...")

        elif re.match(r"\s*}", line):

            # skip final "obj:" or "..." matches
            m = len(sec)
            for s in sec[::-1]:
                if "obj:" not in s and "..." not in s:
                    break
                m -= 1
            sec = sec[:m]

            sec.append(line)

            txt = "\n".join(sec)
            sec = []
            if not args.exclude_regex or not re.search(args.exclude_regex, txt):
                md5 = hashlib.md5(txt.encode())
                d[md5] = txt
                n += 1

        else:
            sec.append(line.rstrip())

    return d


def write_output(args, d):
    """Write output file. `d` is the dict returned by parse_input()."""
    fout = args.outfile
    if args.head:
        fout.write(args.head.read() + "\n")
    fout.write("# ------------------------------------------\n")
    fout.write("# valgrind suppressions generated from\n")
    fout.write("# %s\n" % args.infile.name)
    fout.write("# ------------------------------------------\n")
    for s in d.values():
        fout.write(str(s))
    if args.tail:
        fout.write(args.tail.read())


if __name__ == "__main__":
    main()
