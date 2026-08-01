- Format C++ changes using the `.clang-format` file. You can use `bash task_format_check.sh` to
  check or `bash task_format.sh` to fix all formatting.
- Compile via `ninja -C build/`
- Run tests via `./build/cpp/silva_test`
- Run the regession tests via `bash task_test.sh debug`. This writes a file
  `var/task_demo.sh.output` which is meant to be compared against `task_demo.sh.output`. Some diffs
  here are usually fine.
