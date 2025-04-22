# Syntax Module

## Topological Order of Dependency Graph

* [syntax_ward.hpp](syntax_ward.hpp)
* [tokenization.hpp](tokenization.hpp)
* [parse_tree.hpp](parse_tree.hpp)
* [parse_tree_nursery.hpp](parse_tree_nursery.hpp)
* [seed.hpp](seed.hpp)
* [seed_axe.hpp](seed_axe.hpp)
* [seed_engine.hpp](seed_engine.hpp)

```mermaid
classDiagram
    syntax_ward_t *-- "many" tokenization_t
    syntax_ward_t *-- "many" parse_tree_t
    syntax_ward_t <-- tokenization_t
    tokenization_t <-- parse_tree_t
    parse_tree_t <-- parse_tree_span_t
    seed_engine_t *-- "many" parse_tree_span_t
    syntax_ward_t <-- seed_engine_t
```
