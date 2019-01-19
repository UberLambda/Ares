#!/usr/bin/env python3
# coding: utf-8

# A simple utility scripts to count lines of code in the project.
# See `cloc.sh` for usage.
# FIXME: Document this properly!

import os, sys
import re
from collections import OrderedDict
from argparse import ArgumentParser

class Count:
    def __init__(self, total=0, blank=0, comment=0):
        self.total = total
        self.blank = blank
        self.comment = comment

    @property
    def code(self):
        return self.total - (self.blank + self.comment)

    def __add__(self, other):
        return Count(self.total + other.total, self.blank + other.blank, self.comment + other.comment)

def ignore_re(*line_pats, flags=0):
    line_res = ('^' + line_pat + '$' for line_pat in line_pats)
    return re.compile('|'.join('(' + line_re + ')' for line_re in line_res), flags)


BLANK_MATCHER = re.compile(r'^\s*$')

COMMENT_RES = {  # Name suffix => regex line comment matcher
    ('.cmake', 'CMakeLists.txt', '.py', '.comp'): ignore_re(r'\s*#.*'),
    ('.c', '.cc', '.cpp', '.cxx',
     '.h', '.hh', '.hpp', '.hxx',
     '.inl', '.objc'): ignore_re(r'\s*//.*', '\s*/\*(.*$)*\*/', flags=re.MULTILINE),
}

TEMPLATE_EXT = '.in'

def find_comment_re(filepath):
    if filepath.endswith(TEMPLATE_EXT):
        # Template file, find "real" suffix
        filepath = filepath[:-len(TEMPLATE_EXT)]

    for suffixes, comment_re in COMMENT_RES.items():
        if any(filepath.endswith(suffix) for suffix in suffixes):
            return comment_re
    else:
        return None


def fmt_row(cols, col_lens):
    item_fmts =  (str(col).center(col_lens[i] + 2) for i, col in enumerate(cols))
    return '|' + '|'.join(item_fmts) + '|'


if __name__ == '__main__':
    arg_parser = ArgumentParser(description='Counts lines of code of the files whose names are supplied to stdin')
    arg_parser.add_argument('--by-file', '-B', action='store_true', help='Show LOC on a file-by-file basis')
    args = arg_parser.parse_args()


    filepaths = [line.strip() for line in sys.stdin.readlines()]
    counts = OrderedDict()
    n_counted = 0

    for filepath in filepaths:
        try:
            with open(filepath, 'r') as infile:
                lines = infile.readlines()

            count = Count()
            count.total = len(lines)

            comment_re = find_comment_re(filepath)
            if comment_re:
                count.comment = sum(1 for line in lines if comment_re.match(line))

            count.blank = sum(1 for line in lines if BLANK_MATCHER.match(line))
            counts[filepath] = count
            n_counted += 1

        except (OSError, FileNotFoundError) as e:
            print('Skipping {}: {}'.format(filepath, e), file=sys.stderr)


    counts = OrderedDict(sorted(counts.items(), key=lambda pair: pair[1].total, reverse=True))


    total_count = Count()
    max_filepath_length = 1
   
    for filepath, count in counts.items():
        total_count += count
        max_filepath_length = max(max_filepath_length, len(filepath))


    col_lengths = [max_filepath_length, 10, 10, 10, 10]
    sep_row = fmt_row(['-' * col_len for col_len in col_lengths], col_lengths)

    print(fmt_row(['File', 'Total', 'Code', 'Blank', 'Comment'], col_lengths))
    print(sep_row)

    if args.by_file:
        # Show by-file stats
        for filepath, count in counts.items():
            print(fmt_row([filepath, count.total, count.code, count.blank, count.comment], col_lengths))

        print(sep_row)

    print(fmt_row(['', total_count.total, total_count.code, total_count.blank, total_count.comment], col_lengths))

    print('Done. {} files counted'.format(n_counted))
    n_skipped = len(filepaths) - n_counted
    if n_skipped > 0:
        print('{} file[s] had to be skipped!'.format(n_skipped), file=sys.stderr)
        sys.exit(1)
    # Else exit normally
