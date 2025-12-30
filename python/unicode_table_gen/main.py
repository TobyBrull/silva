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
    values: list[str] = dataclasses.field(default_factory=lambda: ["Invalid"] * 0x110000)

    def recognise(self, codepoint_str: str, value: str):
        if ".." in codepoint_str:
            codepoint_range = codepoint_str.split('..', 1)
            assert len(codepoint_range) == 2
            codepoint_from = int(codepoint_range[0], 16)
            codepoint_upto = int(codepoint_range[1], 16)
            for codepoint in range(codepoint_from, codepoint_upto + 1):
                self.recognise_impl(codepoint, value)
        else:
            codepoint = int(codepoint_str, 16)
            self.recognise_impl(codepoint, value)

    def recognise_one(self, codepoint_str: str, value: str):
        codepoint = int(codepoint_str, 16)
        self.recognise_impl(codepoint, value)

    def recognise_impl(self, codepoint: int, value: str):
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


@dataclasses.dataclass
class PropertyMapping:
    new_name: str | None = None
    value_mapping: dict[str, str] = dataclasses.field(default_factory=dict)

    def apply(self, prop: UnicodeProperty) -> UnicodeProperty:
        retval = UnicodeProperty()
        retval.name = self.new_name or prop.name
        for codepoint, value in enumerate(prop.values):
            new_value = self.value_mapping.get(value, 'Invalid')
            retval.recognise_impl(codepoint, new_value)
        return retval


YES_map = {"Invalid": "No", "YES": "Yes"}
used_properties_mappings = {
    "General_Category": PropertyMapping(
        value_mapping={
            "Pe": "PunctuationEnd",
            "Ps": "PunctuationStart",
        },
    ),
    'XID_Start': PropertyMapping(value_mapping=YES_map),
    'XID_Continue': PropertyMapping(value_mapping=YES_map),
    'Math': PropertyMapping(value_mapping=YES_map),
    'NFC_QC': PropertyMapping(
        new_name="NFC_Quick_Check",
        value_mapping={"Invalid": "Yes", "N": "No", "M": "No"},
    ),
}


class MultistageTable:
    def __init__(
        self, num_lower_bits: int, stage_1: list[int], stage_2: list[int], stage_3: list[dict]
    ):
        self.num_lower_bits = num_lower_bits
        self.stage_1 = stage_1
        self.stage_2 = stage_2
        self.stage_3 = stage_3

    def stats(self) -> str:
        total_size = len(self.stage_1) + len(self.stage_2) + len(self.stage_3) * len(self.stage_3[0])
        return f'{len(self.stage_1)=} {len(self.stage_2)=} {len(self.stage_3)=} = {total_size=}'


def make_multi_stage_table(num_lower_bits: int, props: list[UnicodeProperty]) -> MultistageTable:
    len_stage_1 = 0x110000 >> num_lower_bits
    block_len_stage_2 = 1 << num_lower_bits
    print(f'{len_stage_1=} {block_len_stage_2=}')
    stage_1 = [0] * len_stage_1
    stage_2_blocks: dict[tuple, int] = {}
    stage_2 = []
    stage_3_props: dict[frozenset, int] = {}
    stage_3 = []

    # for codepoint in range(0x11000):
    for stage_1_idx in range(len_stage_1):
        new_block = []
        for stage_2_block_idx in range(block_len_stage_2):
            codepoint = (stage_1_idx << num_lower_bits) | stage_2_block_idx

            curr_props = {}
            for prop in props:
                curr_props[prop.name] = prop.values[codepoint]
            curr_props = frozenset(curr_props.items())

            if curr_props in stage_3_props:
                stage_3_idx = stage_3_props[curr_props]
            else:
                stage_3_idx = len(stage_3)
                stage_3_props[curr_props] = stage_3_idx
                stage_3.append(curr_props)

            new_block.append(stage_3_idx)

        new_block_tuple = tuple(new_block)
        if new_block_tuple in stage_2_blocks:
            stage_2_idx = stage_2_blocks[new_block_tuple]
        else:
            stage_2_idx = len(stage_2) // block_len_stage_2
            stage_2_blocks[new_block_tuple] = stage_2_idx
            stage_2.extend(new_block)

        stage_1[stage_1_idx] = stage_2_idx

    return MultistageTable(num_lower_bits, stage_1, stage_2, stage_3)


def handle_generate(args):
    props_1 = parse_derived_file(os.path.join(args.workdir, unicode_files[0]))
    props_2 = parse_derived_file(os.path.join(args.workdir, unicode_files[1]))
    props_3 = parse_unicode_data(os.path.join(args.workdir, unicode_files[2]))
    props = props_1 + props_2 + props_3

    cleaned_props = []
    for prop in props:
        if prop.name in used_properties_mappings:
            prop_mapping = used_properties_mappings[prop.name]
            cleaned_props.append(prop_mapping.apply(prop))

    for prop in cleaned_props:
        print("===========================")
        print(prop.name)
        print(sorted(set(prop.values)))

    mst = make_multi_stage_table(args.num_lower_bits, cleaned_props)
    print(mst.stats())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--workdir', type=str, required=True)
    subparsers = parser.add_subparsers(dest="command", required=True)
    download_parser = subparsers.add_parser('download')
    download_parser.set_defaults(func=handle_download)
    generate_parser = subparsers.add_parser('generate')
    generate_parser.set_defaults(func=handle_generate)
    generate_parser.add_argument('--num-lower-bits', type=int, default=8)
    args = parser.parse_args()
    assert os.path.exists(args.workdir) and os.path.isdir(
        args.workdir
    ), f'Could not find {args.workdir=}'
    args.func(args)


if __name__ == "__main__":
    main()
