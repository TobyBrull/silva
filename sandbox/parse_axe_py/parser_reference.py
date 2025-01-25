#!/usr/bin/env python
import misc
import testset
import parse_axe


def reference(paxe: parse_axe.ParseAxe, tt: misc.Tokenization) -> str | None:
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
