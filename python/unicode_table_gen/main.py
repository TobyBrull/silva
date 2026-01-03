#!/usr/bin/env python

import os
import csv
import requests
import argparse
import collections
import dataclasses
import pprint

unicode_files = (
    "DerivedNormalizationProps.txt",
    "DerivedCoreProperties.txt",
    "UnicodeData.txt",
)

Codepoint = int


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


def codepoint_to_str(cp: Codepoint) -> str:
    retval = ''
    try:
        retval = chr(cp)
    except:
        pass
    return retval


def str_to_codepoint(cp_str: str) -> Codepoint:
    assert len(cp_str) == 1
    return ord(cp_str[0])


class UnicodeProperty:
    def __init__(self, init_value: str = "Invalid"):
        self.values: list[str] = [init_value] * 0x110000

    def recognise(self, codepoint_str: str, value: str):
        if ".." in codepoint_str:
            codepoint_range = codepoint_str.split('..', 1)
            assert len(codepoint_range) == 2
            codepoint_from = Codepoint(codepoint_range[0], 16)
            codepoint_upto = Codepoint(codepoint_range[1], 16)
            for cp in range(codepoint_from, codepoint_upto + 1):
                self.recognise_impl(cp, value)
        else:
            cp = Codepoint(codepoint_str, 16)
            self.recognise_impl(cp, value)

    def recognise_one(self, codepoint_str: str, value: str):
        cp = Codepoint(codepoint_str, 16)
        self.recognise_impl(cp, value)

    def recognise_impl(self, cp: Codepoint, value: str):
        self.values[cp] = value

    def to_string(self) -> str:
        mm: dict[str, list[Codepoint]] = collections.defaultdict(list)
        for cp, val in enumerate(self.values):
            mm[val].append(cp)
        retval = ""
        for k, codepoints in mm.items():
            retval += f"{k:20} [{len(codepoints):7}] "
            for cp in codepoints[:10]:
                retval += f"0x{cp:04x}, "
            retval += "\n"
        return retval

    def compute_enum_ints(self) -> dict[str, int]:
        enum_values: dict[str, bool] = {}
        for cp, val in enumerate(self.values):
            enum_values[val] = True
        retval: dict[str, int] = {}
        for idx, ev in enumerate(enum_values):
            retval[ev] = idx
        return retval

    def to_cpp(self, include_filename: str) -> tuple[str, str]:
        enum_ints = self.compute_enum_ints()

        hpp = ""
        hpp += "#include <cstdint>\n"
        hpp += "#include <vector>\n"
        hpp += "\n"
        hpp += "namespace silva {\n"
        hpp += "  enum class fragment_category_t : int8_t {\n"
        for ev, idx in enum_ints.items():
            hpp += f"    {ev} = {idx},\n"
        hpp += "  };\n"
        hpp += "\n"
        hpp += "  extern std::vector<fragment_category_t> fragment_category_by_codepoint;\n"
        hpp += "}\n"

        cpp = ""
        cpp += f"#include \"{include_filename}\"\n"
        cpp += "\n"
        cpp += "namespace silva {\n"
        cpp += "  const int8_t impl[] = {"
        for cp, val in enumerate(self.values):
            if cp % 32 == 0:
                cpp += "\n    "
            cpp += f"{enum_ints[val]},"
        cpp += "\n  };\n"
        cpp += "\n"
        cpp += "  std::vector<fragment_category_t> fragment_category_by_codepoint = [] {\n"
        cpp += "    std::vector<fragment_category_t> retval;\n"
        cpp += "    retval.reserve(std::size(impl));\n"
        cpp += "    for (const auto x: impl) {\n"
        cpp += "      retval.push_back(fragment_category_t{x});\n"
        cpp += "    }\n"
        cpp += "    return retval;\n"
        cpp += "  }();\n"
        cpp += "}\n"
        return hpp, cpp

    def get_by_value(self, value: str) -> list[Codepoint]:
        retval = []
        for cp, val in enumerate(self.values):
            if val == value:
                retval.append(cp)
        return retval

    def get_mask(self, value: str) -> list[bool]:
        return [x == value for x in self.values]

    def update_masked(self, mask, new_value):
        assert len(mask) == len(self.values)
        for idx in range(len(mask)):
            if mask[idx]:
                self.values[idx] = new_value

    def value_set(self) -> list[str]:
        return list(sorted(set(self.values)))

    def ingest(self, other: 'UnicodeProperty', other_value: str, self_value: str):
        for cp, val in enumerate(other.values):
            if val == other_value:
                self.values[cp] = self_value

    @staticmethod
    def combine(lhs: "UnicodeProperty", rhs: "UnicodeProperty") -> "UnicodeProperty":
        retval = UnicodeProperty()
        for cp, (val1, val2) in enumerate(zip(lhs.values, rhs.values)):
            retval.recognise_impl(cp, val1 + val2)
        return retval


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


def parse_derived_file(filename: str) -> dict[str, UnicodeProperty]:
    propname_map: dict[str, UnicodeProperty] = collections.defaultdict(UnicodeProperty)
    rows = _load_derived_file(filename)
    for row in rows:
        if len(row) == 2:
            propname_map[row[1]].recognise(row[0], "YES")
        elif len(row) == 3:
            propname_map[row[1]].recognise(row[0], row[2])
        else:
            assert False, f'expected each row to have 2 or 3 fields'
    retval = {}
    for prop_name, prop in propname_map.items():
        retval["Derived_" + prop_name] = prop
    return retval


@dataclasses.dataclass
class RangeStart:
    name: str
    codepoint_str: str
    value: str


def parse_unicode_data(filename: str) -> dict[str, UnicodeProperty]:
    prop_gen_cat = UnicodeProperty()
    prop_names = UnicodeProperty()
    with open(filename, 'r') as file:
        reader = csv.reader(file, delimiter=';')
        range_start: RangeStart | None = None
        for line_num, row in enumerate(reader):
            row_name = row[1]
            if (row_name[0] == "<" and row_name[-1] == ">") and row_name != "<control>":
                row_name = row_name[1:-1]
                if row_name.endswith("First"):
                    assert range_start is None
                    range_start = RangeStart(name=row_name[:-5], codepoint_str=row[0], value=row[2])
                elif row_name.endswith("Last"):
                    assert range_start is not None
                    assert range_start.name + "Last" == row_name
                    assert range_start.value == row[2]
                    prop_gen_cat.recognise(f"{range_start.codepoint_str}..{row[0]}", row[2])
                    prop_gen_cat.recognise(f"{range_start.codepoint_str}..{row[0]}", row[1])
                    range_start = None
                else:
                    raise Exception(
                        f"Error in processing range in {filename=} {row_name=} {line_num=}"
                    )
            else:
                assert range_start is None, f"{line_num=}"
                prop_gen_cat.recognise_one(row[0], row[2])
                prop_names.recognise_one(row[0], row[1])
    return {"Names": prop_names, "General_Category": prop_gen_cat}


def make_mapping(map: dict[str, str], default: str = "Invalid"):
    def retval(x: str) -> str:
        return map.get(x, default)

    return retval


def apply_mapping(prop: UnicodeProperty, mapping) -> UnicodeProperty:
    retval = UnicodeProperty()
    for cp in range(len(prop.values)):
        value = prop.values[cp]
        new_value = mapping(value)
        retval.recognise_impl(cp, new_value)
    return retval


YES_mapping = make_mapping({"Invalid": "No", "YES": "Yes"})


def process_props(loaded_props: dict[str, UnicodeProperty]) -> dict[str, UnicodeProperty]:
    retval: dict[str, UnicodeProperty] = {}

    prop_gen_cat = apply_mapping(
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
                "Cf": "Unassigned",  # "ControlFormat",
                "Cs": "Unassigned",  # "ControlSurrogate",
                "Co": "Unassigned",  # "ControlPrivateUse",
                "Cn": "Unassigned",  # "ControlNotAssigned",
                "Invalid": "Unassigned",
            },
            "Unassigned",
        ),
    )
    retval["General_Category"] = prop_gen_cat

    retval["XID_Start"] = apply_mapping(loaded_props["Derived_XID_Start"], YES_mapping)
    retval["XID_Continue"] = apply_mapping(loaded_props["Derived_XID_Continue"], YES_mapping)

    # retval["Math"] = apply_mapping(loaded_props["Derived_Math"], YES_mapping)
    retval["Operator"] = apply_mapping(
        prop_gen_cat,
        make_mapping(
            {
                "PunctuationConnector": "Yes",
                "PunctuationDash": "Yes",
                "PunctuationOpen": "Yes",
                "PunctuationClose": "Yes",
                "PunctuationInitial": "Yes",
                "PunctuationFinal": "Yes",
                "PunctuationOther": "Yes",
                "SymbolMath": "Yes",
                "SymbolCurrency": "Yes",
                "SymbolModifier": "Yes",
                "SymbolOther": "Yes",
            },
            "No",
        ),
    )

    retval["NFC_QuickCheck"] = apply_mapping(
        loaded_props["Derived_NFC_QC"], make_mapping({"N": "No", "M": "No"}, "Yes")
    )

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

    def to_cpp(self, include_filename: str) -> tuple[str, str]:
        hpp = ""
        hpp += "// clang-format off\n"
        hpp += "#include \"canopy/unicode.hpp\"\n"
        hpp += "\n"
        hpp += "namespace silva {\n"
        hpp += "  enum class fragment_category_t {\n"
        for idx, value in enumerate(self.stage_3):
            hpp += f"    {value} = {idx},\n"
        hpp += "  };\n"
        hpp += "\n"
        hpp += "  extern unicode::table_t<fragment_category_t> fragment_table;\n"
        hpp += "}\n"

        cpp = ""
        def array_to_str(arr: list):
            retval = ""
            for cp, val in enumerate(arr):
                if cp % 32 == 0:
                    retval += "\n      "
                retval += f"{val},"
                if cp % 32 != 31:
                    retval += " "
            return retval

        cpp += "// clang-format off\n"
        cpp += f"#include \"{include_filename}\"\n"
        cpp += "\n"
        cpp += "namespace silva {\n"
        cpp += "  using enum fragment_category_t;\n"
        cpp += "\n"
        cpp += "  unicode::table_t<fragment_category_t> fragment_table {\n"
        cpp += "    .stage_1 = {" + array_to_str(self.stage_1) + "},\n"
        cpp += "    .stage_2 = {" + array_to_str(self.stage_2) + "},\n"
        cpp += "    .stage_3 = {" + array_to_str(self.stage_3) + "},\n"
        cpp += "  };"
        cpp += "}\n"
        return hpp, cpp


def make_multi_stage_table(num_lower_bits: int, prop: UnicodeProperty) -> MultistageTable:
    len_stage_1 = 0x110000 >> num_lower_bits
    block_len_stage_2 = 1 << num_lower_bits
    print(f'{len_stage_1=} {block_len_stage_2=}')
    stage_1 = [0] * len_stage_1
    stage_2_blocks: dict[tuple, int] = {}
    stage_2 = []
    stage_3_props: dict[str, int] = {}
    stage_3 = []

    # for codepoint in range(0x11000):
    for stage_1_idx in range(len_stage_1):
        new_block = []
        for stage_2_block_idx in range(block_len_stage_2):
            codepoint = (stage_1_idx << num_lower_bits) | stage_2_block_idx
            curr_props = prop.values[codepoint]

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
            stage_2_idx = len(stage_2)
            stage_2_blocks[new_block_tuple] = stage_2_idx
            stage_2.extend(new_block)

        stage_1[stage_1_idx] = stage_2_idx

    return MultistageTable(num_lower_bits, stage_1, stage_2, stage_3)


@dataclasses.dataclass
class Parentheses:
    name: str = ""
    left: Codepoint = -1
    right: Codepoint = -1

    def is_complete(self):
        return self.left != -1 and self.right != -1

    def __repr__(self):
        retval = ""
        retval += f"0x{self.left:04x} 0x{self.right:04x} "
        retval += f"{codepoint_to_str(self.left)}...{codepoint_to_str(self.right)}"
        retval += f" : {self.name}"
        return retval


def discover_parentheses(
    general_category: UnicodeProperty, codepoint_names: list[str]
) -> tuple[list[Parentheses], UnicodeProperty]:
    left_right: dict[str, Parentheses] = collections.defaultdict(Parentheses)
    for cp, name in enumerate(codepoint_names):
        if "LEFT" in name and "RIGHT" in name:
            continue
        if "LEFT" in name:
            name = name.replace("LEFT", "LEFT/RIGHT")
            left_right[name].left = cp
        elif "RIGHT" in name:
            name = name.replace("RIGHT", "LEFT/RIGHT")
            left_right[name].right = cp

    def is_punct(cp: Codepoint) -> bool:
        gencat = general_category.values[cp]
        return gencat == "Ps" or gencat == "Pe" or gencat == "Pi" or gencat == "Pf"

    parens: list[Parentheses] = []
    for name, pp in left_right.items():
        if pp.is_complete() and is_punct(pp.left) and is_punct(pp.right):
            parens.append(Parentheses(name=name, left=pp.left, right=pp.right))
    parens = sorted(parens, key=lambda pp: pp.left)

    parens_prop = UnicodeProperty("No")
    for pp in parens:
        parens_prop.recognise_impl(pp.left, "Left")
        parens_prop.recognise_impl(pp.right, "Right")

    return parens, parens_prop


def handle_generate(args):
    # Load UCD data.
    props_1 = parse_derived_file(os.path.join(args.workdir, unicode_files[0]))
    props_2 = parse_derived_file(os.path.join(args.workdir, unicode_files[1]))
    props_3 = parse_unicode_data(os.path.join(args.workdir, unicode_files[2]))
    loaded_props = props_1 | props_2 | props_3

    # Process UCD data.
    cleaned_props = process_props(loaded_props)
    general_category = loaded_props["General_Category"]
    parens, parens_prop = discover_parentheses(general_category, loaded_props["Names"].values)
    cleaned_props["IsParenthesis"] = parens_prop

    # Print processed UCD data.
    for name, prop in cleaned_props.items():
        print(f"===== {name} =======")
        print(prop.to_string())
    pprint.pprint(parens[:10])
    print()

    prop_comb = UnicodeProperty.combine(cleaned_props["XID_Start"], cleaned_props["XID_Continue"])
    assert "YesNo" not in prop_comb.value_set(), f'Expected XID_Start to be subset of XID_Continue'

    prop_comb = UnicodeProperty.combine(cleaned_props["XID_Continue"], cleaned_props["Operator"])
    assert prop_comb.get_by_value("YesYes") == [
        str_to_codepoint(x)
        for x in [
            '_',
            '·',
            '·',
            '‿',
            '⁀',
            '⁔',
            '℘',
            '℮',
            '・',
            '︳',
            '︴',
            '﹍',
            '﹎',
            '﹏',
            '＿',
            '･',
        ]
    ], f'Expected only one element in intersection of XID_Continue and Operator'

    main_prop = UnicodeProperty("Forbidden")
    main_prop.ingest(cleaned_props["Operator"], "Yes", "Operator")
    main_prop.ingest(cleaned_props["IsParenthesis"], "Left", "ParenthesisLeft")
    main_prop.ingest(cleaned_props["IsParenthesis"], "Right", "ParenthesisRight")
    main_prop.ingest(cleaned_props["XID_Continue"], "Yes", "XID_Continue")
    main_prop.ingest(cleaned_props["XID_Start"], "Yes", "XID_Start")
    main_prop.values[str_to_codepoint('_')] = "XID_Start"
    main_prop.ingest(cleaned_props["General_Category"], "Unassigned", "Forbidden")
    main_prop.ingest(cleaned_props["NFC_QuickCheck"], "No", "Forbidden")
    main_prop.values[str_to_codepoint('\n')] = "Newline"
    main_prop.values[str_to_codepoint(' ')] = "Space"

    assert main_prop.values[str_to_codepoint('_')] == "XID_Start"
    assert main_prop.values[str_to_codepoint('a')] == "XID_Start"
    assert main_prop.values[str_to_codepoint('0')] == "XID_Continue"
    assert main_prop.values[str_to_codepoint('"')] == "Operator"
    assert main_prop.values[str_to_codepoint('*')] == "Operator"
    assert main_prop.values[str_to_codepoint('!')] == "Operator"
    assert main_prop.values[str_to_codepoint('⊙')] == "Operator"

    print(main_prop.to_string())

    hpp_filename = f'{args.output_file_base}.hpp'
    cpp_filename = f'{args.output_file_base}.cpp'
    include_filename = os.path.basename(hpp_filename)

    # hpp, cpp = main_prop.to_cpp(include_filename)

    mst = make_multi_stage_table(args.num_lower_bits, main_prop)
    print(mst.stats())
    hpp, cpp = mst.to_cpp(include_filename)

    with open(hpp_filename, 'w') as fh:
        fh.write(hpp)
    with open(cpp_filename, 'w') as fh:
        fh.write(cpp)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--workdir', type=str, required=True)
    subparsers = parser.add_subparsers(dest="command", required=True)
    download_parser = subparsers.add_parser('download')
    download_parser.set_defaults(func=handle_download)
    generate_parser = subparsers.add_parser('generate')
    generate_parser.set_defaults(func=handle_generate)
    generate_parser.add_argument('--num-lower-bits', type=int, default=8)
    generate_parser.add_argument('--output-file-base', type=str, required=True)
    args = parser.parse_args()
    assert os.path.exists(args.workdir) and os.path.isdir(
        args.workdir
    ), f'Could not find {args.workdir=}'
    args.func(args)


if __name__ == "__main__":
    main()
