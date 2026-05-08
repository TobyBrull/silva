# Cedar tests

```bash
rm -rf var/wacct/ var/cedar-tests/ && python python/cedar_tests/run.py setup
ninja -C build/ && python python/cedar_tests/run.py run-tests --output-file-list var/failed.txt
```
