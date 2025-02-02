* make tests exhaustive
* support:
    * "a < b < c"

```python
import ast
print(ast.dump(ast.parse("a < b < c < d < e < f", mode='eval'), indent=2))
print(ast.dump(ast.parse("a < b < c < d < (e < f)", mode='eval'), indent=2))
a, b, c = 1, 10, 5
print(a < b < c)
print((a < b) < c)
```
