# unicode_table_gen

[Article I](https://here-be-braces.com/fast-lookup-of-unicode-properties/)
[Article II](https://www.strchr.com/multi-stage_tables)

```bash
pixi run -e python-only python tools/unicode_table_gen/main.py --workdir=var/ download
pixi run -e python-only python tools/unicode_table_gen/main.py --workdir=var/ generate --output-file-base src/syntax/fragmentization_data
```
