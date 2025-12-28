#!/usr/bin/env python

import os
import csv
import requests
import argparse
import collections
import dataclasses

unicode_files = (
    "DerivedNormalizationProps.txt",
    "DerivedCoreProperties.txt",
    "UnicodeData.txt",
)

excluded_derived_properties = (
    'FC_NFKC',
    'NFKC_CF',
    "NFKC_SCF",
)


def handle_download(args):
    for unicode_file in unicode_files:
        url = f'https://www.unicode.org/Public/UCD/latest/ucd/{unicode_file}'
        target_filename = os.path.join(args.workdir, unicode_file)

        # Use stream=True to handle large files efficiently
        with requests.get(url, stream=True) as response:
            response.raise_for_status()  # Check for HTTP errors
            with open(target_filename, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    f.write(chunk)

        print(f"Downloaded: {target_filename}")


@dataclasses.dataclass
class UnicodeProperty:
    name: str = ""
    value_set: set[str] = dataclasses.field(default_factory=lambda: set([""]))
    values: list[str] = dataclasses.field(default_factory=lambda: [""] * 0x110000)

    def recognise(self, codepoint_str: str, value: str):
        if ".." in codepoint_str:
            codepoint_range = codepoint_str.split('..', 1)
            assert len(codepoint_range) == 2
            codepoint_from = int(codepoint_range[0], 16)
            codepoint_upto = int(codepoint_range[1], 16)
            for codepoint in range(codepoint_from, codepoint_upto + 1):
                self._recognise_impl(codepoint, value)
        else:
            codepoint = int(codepoint_str, 16)
            self._recognise_impl(codepoint, value)

    def recognise_one(self, codepoint_str: str, value: str):
        codepoint = int(codepoint_str, 16)
        self._recognise_impl(codepoint, value)

    def _recognise_impl(self, codepoint: int, value: str):
        self.value_set.add(value)
        self.values[codepoint] = value


def _load_derived_file(filename: str) -> list[tuple[str, ...]]:
    results = []
    with open(filename, 'r') as file:
        for line in file:
            content = line.split('#', 1)[0].strip()
            if not content:
                continue
            fields = tuple(field.strip() for field in content.split(';'))
            results.append(fields)
    return results


def parse_derived_file(filename: str) -> list[UnicodeProperty]:
    propname_map = collections.defaultdict(UnicodeProperty)
    rows = _load_derived_file(filename)
    for row in rows:
        if len(row) == 2:
            propname_map[row[1]].recognise(row[0], "YES")
        elif len(row) == 3:
            propname_map[row[1]].recognise(row[0], row[2])
        else:
            assert False, f'expected each row to have 2 or 3 fields'
    retval = []
    for k, v in propname_map.items():
        if k not in excluded_derived_properties:
            v.name = k
            retval.append(v)
    return retval


def parse_unicode_data(filename: str) -> list[UnicodeProperty]:
    retval = UnicodeProperty(name="General_Category")
    with open(filename, 'r') as file:
        reader = csv.reader(file, delimiter=';')
        for row in reader:
            retval.recognise_one(row[0], row[2])
    return [retval]


def handle_generate(args):
    props_1 = parse_derived_file(os.path.join(args.workdir, unicode_files[0]))
    props_2 = parse_derived_file(os.path.join(args.workdir, unicode_files[1]))
    props_3 = parse_unicode_data(os.path.join(args.workdir, unicode_files[2]))
    props = props_1 + props_2 + props_3

    for prop in props:
        print("===========================")
        print(prop.name)
        print(prop.value_set)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--workdir', type=str, required=True)
    subparsers = parser.add_subparsers(dest="command", required=True)
    download_parser = subparsers.add_parser('download')
    download_parser.set_defaults(func=handle_download)
    generate_parser = subparsers.add_parser('generate')
    generate_parser.set_defaults(func=handle_generate)
    args = parser.parse_args()
    assert os.path.exists(args.workdir) and os.path.isdir(
        args.workdir
    ), f'Could not find {args.workdir=}'
    args.func(args)


if __name__ == "__main__":
    main()
