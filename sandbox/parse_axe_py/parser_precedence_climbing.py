#!/usr/bin/env python
import misc
import parse_axe
import testset


def expr_impl(
    paxe: parse_axe.ParseAxe, tokens: list[misc.Token], index: int, curr_prec: int
) -> tuple[str, int]:
    assert index < len(tokens)
    assert tokens[index].type == misc.TokenType.ATOM
    result = tokens[index].value
    index += 1

    while index < len(tokens):
        assert tokens[index].type == misc.TokenType.OPER
        op_name = tokens[index].value
        res = paxe.prec_infix(op_name)
        assert res is not None
        (op_prec, used_prec) = res
        if op_prec < curr_prec:
            break
        index += 1
        rhs, index = expr_impl(paxe, tokens, index, used_prec)
        result = misc.cons_str(op_name, result, rhs)

    return result, index


def precedence_climbing(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    retval, index = expr_impl(paxe, tokens, 0, 0)
    assert index == len(tokens)
    return retval


def _run():
    testset.execute(
        precedence_climbing,
        excluded=[
            'base/allfix',
            'base/parentheses',
            'base/subscript',
            'base/ternary',
            'pq/allfix',
        ],
    )


if __name__ == '__main__':
    _run()
