# QBE tests

```bash
rm -rf tmp/qbe_repo/ tmp/qbe_tests/ && pixi run -e python-only -- python tools/qbe_tests/run.py setup
ninja -C build.default.release/ && pixi run -e python-only -- python tools/qbe_tests/run.py run-tests --output-file-list tmp/failed.txt
```
