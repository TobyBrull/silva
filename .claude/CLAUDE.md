# Coding style C++
- Always use curly brackets after `if` `else` statements and `for` `while` loops.

# Validation
- Compile via `ninja -C build/`
- Run tests via `./build/cpp/silva_test`
- Run the regession tests via `bash demo.sh > demo.sh.output`. This shouldn't change the file
"demo.sh.output" too much, but a few changes are often expected there. At the very least, the script
should run to completion.
