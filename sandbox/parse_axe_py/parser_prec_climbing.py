#!/usr/bin/env python
import dataclasses
import pprint
import enum

import misc
import testset
import parse_axe
import parse_tree

Node = parse_tree.Node


@dataclasses.dataclass
class ExprImplResult:
    node: Node


def expr_impl(paxe: parse_axe.ParseAxe, tokens: list[misc.Token], begin: int) -> ExprImplResult:
    return ExprImplResult(Node())


def prec_climbing(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> Node:
    retval = expr_impl(paxe, tokens, 0)
    return retval.node


def _run():
    testset.execute(prec_climbing, excluded=[
        'base/infix',
        'base/allfix',
        'base/parentheses',
        'base/subscript',
        'base/ternary',
        'low-postfix/flat',
        'pq/allfix',
        'ternary/easy',
        'parens/easy',
        'parens-concat/easy',
        'parens-concat-2/easy',
        'concat/easy',
        'concat_rtl/easy',
        'C++/basic',
    ])


if __name__ == '__main__':
    _run()
