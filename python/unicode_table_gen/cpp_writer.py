import os

class CppWriter:
    def __init__(self, output_file_base: str):
        self.output_file_base = output_file_base
        self.hpp_filename = f'{self.output_file_base}.hpp'
        self.cpp_filename = f'{self.output_file_base}.cpp'
        self.include_filename = os.path.basename(self.hpp_filename)

        self.hpp = ""
        self.cpp = ""

    def init(self):
        self.hpp += "// GENERATED: DO NOT EDIT!\n"
        self.hpp += "//\n"
        self.hpp += "// clang-format off\n"
        self.hpp += "#include \"canopy/hash.hpp\"\n"
        self.hpp += "#include \"canopy/unicode.hpp\"\n"
        self.hpp += "\n"
        self.hpp += "namespace silva {\n"

        self.cpp += "// GENERATED: DO NOT EDIT!\n"
        self.cpp += "//\n"
        self.cpp += "// clang-format off\n"
        self.cpp += f"#include \"{self.include_filename}\"\n"
        self.cpp += "\n"
        self.cpp += "namespace silva {\n"
        self.cpp += "  using enum fragment_category_t;\n"
        self.cpp += "\n"

    def finalize(self):
        self.hpp += "}\n"

        self.cpp += "}\n"

    def write(self):
        with open(self.hpp_filename, 'w') as fh:
            fh.write(self.hpp)
        with open(self.cpp_filename, 'w') as fh:
            fh.write(self.cpp)
