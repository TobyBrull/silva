# unicode_table_gen

[Article I](https://here-be-braces.com/fast-lookup-of-unicode-properties/)
[Article II](https://www.strchr.com/multi-stage_tables)

```bash
pixi run -e python-only python python/unicode_table_gen/main.py --workdir=var/ download
pixi run -e python-only python python/unicode_table_gen/main.py --workdir=var/ generate --output-file-base cpp/syntax/fragmentization_data
```
