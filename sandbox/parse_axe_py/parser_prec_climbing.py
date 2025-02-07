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
    end: int


ATOM_MODE = parse_axe.ParseMode.ATOM
INFIX_MODE = parse_axe.ParseMode.INFIX


def should_squash(stack_level: parse_axe.LevelInfo, new_level: parse_axe.LevelInfo) -> bool:
    if stack_level.prec > new_level.prec:
        return True
    elif stack_level.prec < new_level.prec:
        return False
    else:
        assert stack_level.assoc == new_level.assoc
        assoc = stack_level.assoc
        return not (assoc == parse_axe.Assoc.RIGHT_TO_LEFT)


def expr_impl(
    paxe: parse_axe.ParseAxe,
    tokens: list[misc.Token],
    index: int,
    mode: parse_axe.ParseMode,
    base_level_info: parse_axe.LevelInfo,
) -> ExprImplResult:

    curr_node = None

    

    assert curr_node is not None
    return ExprImplResult(curr_node, index)


def prec_climbing(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> Node:
    retval = expr_impl(
        paxe,
        tokens,
        0,
        ATOM_MODE,
        parse_axe.LevelInfo('END', -1, parse_axe.Assoc.NONE),
    )
    return retval.node


def _run():
    testset.execute(
        prec_climbing,
        excluded=[
            'base/infix',
            'base/infix-flat',
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
        ],
    )


if __name__ == '__main__':
    _run()
