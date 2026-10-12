# C tests

```bash
rm -rf tmp/wacct/ tmp/c_tests/ && pixi run -e python-only -- python tools/c_tests/run.py setup
ninja -C build.default.release/ && pixi run -e python-only -- python tools/c_tests/run.py run-tests --output-file-list tmp/failed.txt
```
