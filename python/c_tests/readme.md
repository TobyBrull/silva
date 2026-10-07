# C tests

```bash
rm -rf var/wacct/ var/c_tests/ && python python/c_tests/run.py setup
ninja -C build.default.release/ && python python/c_tests/run.py run-tests --output-file-list var/failed.txt
```
