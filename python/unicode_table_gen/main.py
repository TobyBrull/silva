#!/usr/bin/env python

import os
import csv
import requests
import argparse
import collections
import dataclasses
import enum

import numpy as np

from typing import Any

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

    def to_string(self) -> str:
        retval = f"========= {self.name} =========\n"
        mm = collections.defaultdict(list)
        for codepoint, val in enumerate(self.values):
            utf8 = ''
            try:
                utf8 = chr(codepoint)
            except:
                pass
            mm[val].append(utf8)
        for k, v in sorted(mm.items()):
            retval += f'{k:20} [{len(v):7}] {v[:10]}\n'
        retval += "\n"
        return retval

    def get_by_value(self, value: str) -> list[str]:
        retval = []
        for codepoint, val in enumerate(self.values):
            if val == value:
                utf8 = ''
                try:
                    utf8 = chr(codepoint)
                except:
                    pass
                retval.append(utf8)
        return retval

    def get_mask(self, value: str) -> list[bool]:
        return [x == value for x in self.values]

    def update_masked(self, mask, new_value):
        assert len(mask) == len(self.values)
        for idx in range(len(mask)):
            if mask[idx]:
                self.values[idx] = new_value


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
        v.name = "Derived_" + k
        retval.append(v)
    return retval


@dataclasses.dataclass
class RangeStart:
    name: str
    codepoint: str
    value: str


def parse_unicode_data(filename: str) -> tuple[list[str], list[UnicodeProperty]]:
    prop = UnicodeProperty(name="General_Category")
    names = UnicodeProperty(name="Names")
    with open(filename, 'r') as file:
        reader = csv.reader(file, delimiter=';')
        range_start: RangeStart | None = None
        for line_num, row in enumerate(reader):
            row_name = row[1]
            if (row_name[0] == "<" and row_name[-1] == ">") and row_name != "<control>":
                row_name = row_name[1:-1]
                if row_name.endswith("First"):
                    assert range_start is None
                    range_start = RangeStart(name=row_name[:-5], codepoint=row[0], value=row[2])
                elif row_name.endswith("Last"):
                    assert range_start is not None
                    assert range_start.name + "Last" == row_name
                    assert range_start.value == row[2]
                    prop.recognise(f"{range_start.codepoint}..{row[0]}", row[2])
                    prop.recognise(f"{range_start.codepoint}..{row[0]}", row[1])
                    range_start = None
                else:
                    raise Exception(
                        f"Error in processing range in {filename=} {row_name=} {line_num=}"
                    )
            else:
                assert range_start is None, f"{line_num=}"
                prop.recognise_one(row[0], row[2])
                names.recognise_one(row[0], row[1])
    return names.values, [prop]


def make_mapping(map: dict[str, str], default: str = "Invalid"):
    def retval(x: str) -> str:
        return map.get(x, default)

    return retval


def apply_mapping(prop: UnicodeProperty, mapping, new_name: str | None = None) -> UnicodeProperty:
    retval = UnicodeProperty()
    retval.name = new_name or prop.name
    for codepoint in range(len(prop.values)):
        value = prop.values[codepoint]
        new_value = mapping(value)
        retval.recognise_impl(codepoint, new_value)
    return retval


YES_mapping = make_mapping({"Invalid": "No", "YES": "Yes"})


def process_props(loaded_props: dict[str, UnicodeProperty]) -> list[UnicodeProperty]:
    retval: list[UnicodeProperty] = []

    general_cat = apply_mapping(
        loaded_props["General_Category"],
        make_mapping(
            {
                "Lu": "LetterUppercase",
                "Ll": "LetterLowercase",
                "Lt": "LetterTitlecase",
                "Mn": "MarkNonSpacing",
                "Mc": "MarkSpacingCombining",
                "Me": "MarkEnclosing",
                "Nd": "NumberDecimalDigit",
                "Nl": "NumberLetter",
                "No": "NumberOther",
                "Zs": "SeparatorSpace",
                "Zl": "SeparatorLine",
                "Zp": "SeparatorParagraph",
                "Lm": "LetterModifier",
                "Lo": "LetterOther",
                "Pc": "PunctuationConnector",
                "Pd": "PunctuationDash",
                "Ps": "PunctuationOpen",
                "Pe": "PunctuationClose",
                "Pi": "PunctuationOpen",  # "PunctuationInitial",
                "Pf": "PunctuationClose",  # "PunctuationFinal",
                "Po": "PunctuationOther",
                "Sm": "SymbolMath",
                "Sc": "SymbolCurrency",
                "Sk": "SymbolModifier",
                "So": "SymbolOther",
                "Cc": "Unassigned",  # "ControlOther",
                "Cf": "ControlFormat",
                "Cs": "Unassigned",  # "ControlSurrogate",
                "Co": "Unassigned",  # "ControlPrivateUse",
                "Cn": "Unassigned",  # "ControlNotAssigned",
                "Invalid": "Unassigned",
            },
            "Other",
        ),
        new_name="Parentheses",
    )
    retval.append(general_cat)

    retval.append(apply_mapping(loaded_props["Derived_XID_Start"], YES_mapping))
    retval.append(apply_mapping(loaded_props["Derived_XID_Continue"], YES_mapping))
    retval.append(apply_mapping(loaded_props["Derived_Math"], YES_mapping))

    nfc_qc = apply_mapping(
        loaded_props["Derived_NFC_QC"],
        make_mapping(
            {
                "N": "No",
                "M": "No",
            },
            "Yes",
        ),
    )
    nfc_qc.update_masked(general_cat.get_mask("Unassigned"), "No")
    retval.append(nfc_qc)

    return retval


class MultistageTable:
    def __init__(
        self, num_lower_bits: int, stage_1: list[int], stage_2: list[int], stage_3: list[dict]
    ):
        self.num_lower_bits = num_lower_bits
        self.stage_1 = stage_1
        self.stage_2 = stage_2
        self.stage_3 = stage_3

    def stats(self) -> str:
        total_size = (
            len(self.stage_1) + len(self.stage_2) + len(self.stage_3) * len(self.stage_3[0])
        )
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
    codepoint_names, props_3 = parse_unicode_data(os.path.join(args.workdir, unicode_files[2]))
    props = props_1 + props_2 + props_3
    loaded_props: dict[str, UnicodeProperty] = {prop.name: prop for prop in props}

    cleaned_props = process_props(loaded_props)

    for prop in cleaned_props:
        print(prop.to_string())
    print(cleaned_props[0].get_by_value("PunctuationOpen"))
    print(cleaned_props[0].get_by_value("PunctuationClose"))

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
