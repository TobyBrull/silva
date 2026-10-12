- When I say "git staged changes", then I mean the output of "git diff --staged". When I say "git
  unstaged changes", then I mean the output of "git diff".
- In relation to git, never stage any changes, never make any commits, never create any branches,
  never push any branches.
- Never read or write any files outside of the repo root. If you need to write temporary files, put
  them in the "tmp/" subfolder in the repo root. Don't delete temporary files.
- Format C++ changes using the `.clang-format` file. You can use `bash tools/task_format.sh check`
  to check or `bash tools/task_format.sh update` to fix all formatting.
- Compile via `ninja -C build/`.
- Run tests via `./build/cpp/silva_test`.
- Run the regession tests via `bash src/regression_test.sh release`. This writes a file
  `tmp/regression_test.sh.output` which is meant to be compared against
  `src/regression_test.sh.output`. Some diffs here are usually fine.
- Keep comments in the generated code to a minimum.
