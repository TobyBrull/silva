# C tests

```bash
rm -rf tmp/wacct/ tmp/c_tests/ && python tools/c_tests/run.py setup
ninja -C build.default.release/ && python tools/c_tests/run.py run-tests --output-file-list tmp/failed.txt
```
