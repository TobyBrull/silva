# unicode_table_gen

[Article I](https://here-be-braces.com/fast-lookup-of-unicode-properties/)
[Article II](https://www.strchr.com/multi-stage_tables)

```bash
rm -rf venv/
python -m venv venv/
source venv/bin/activate
python -m pip install pytest black pyright requests numpy
python python/unicode_table_gen/main.py --workdir=var/ download
python python/unicode_table_gen/main.py --workdir=var/ generate
```
