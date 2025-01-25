#!/usr/bin/env python
import misc
import testset
import parse_axe


def reference(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str | None:
    for level in reversed(paxe.levels):
        if type(level) == parse_axe.LevelPrefix:
            pass
    return None


def _run():
    with testset.Testset(reference) as ts:
        pass
        # ts.infix_only()
        # ts.allfix()
        # ts.parentheses()
        # ts.subscript()
        # ts.ternary()


if __name__ == '__main__':
    _run()
