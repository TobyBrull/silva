#!/usr/bin/env python3

"""
Collect every `silva::parse_tree_t::to_string()` from stdin and write them into files.
"""

import argparse
import re
import sys

TOKEN_LINE = re.compile(r"^\[\s*(\d+)\]\s")
TREE_LINE = re.compile(r"^\s*\[(\d+)\]\.")


def find_parse_trees(text):
    lines = text.splitlines()
    n = len(lines)
    retval = []
    i = 0
    while i < n:
        # consume tokenization
        m = TOKEN_LINE.match(lines[i])
        if not (m and int(m.group(1)) == 0):
            i += 1
            continue
        start = i
        expected = 0
        j = i
        while j < n:
            mm = TOKEN_LINE.match(lines[j])
            if mm and int(mm.group(1)) == expected:
                expected += 1
                j += 1
            else:
                break

        k = j
        while k < n and lines[k].strip() == "":
            k += 1

        # consume parse-tree
        if k < n and TREE_LINE.match(lines[k]):
            while k < n and TREE_LINE.match(lines[k]):
                k += 1
            retval.append("\n".join(lines[start:k]))
            i = k
        else:
            i = j

    return retval


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output-pattern", required=True)
    args = parser.parse_args(argv)

    text = sys.stdin.read()
    trees = find_parse_trees(text)

    if not trees:
        print("No silva::parse_tree_t::to_string output found.", file=sys.stderr)
        return 1

    filenames = []
    for index, tree in enumerate(trees):
        filename = args.output_pattern.format(index, index=index, i=index, n=index)
        with open(filename, "w") as f:
            f.write(tree)
            f.write("\n")
        filenames.append(filename)

    for i in range(0, len(filenames), 2):
        print(f"meld    {' '.join(filenames[i : i + 2])}")
        print(f"nvim -d {' '.join(filenames[i : i + 2])}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
