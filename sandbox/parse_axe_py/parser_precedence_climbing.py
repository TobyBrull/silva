#!/usr/bin/env python
import misc
import parse_axe
import testset


def expr_impl(paxe: parse_axe.ParseAxe, tt: misc.Tokenization, curr_prec: int) -> str:
    assert tt.curr().type == misc.TokenType.ATOM
    result = tt.curr().value
    tt.token_idx += 1

    while not tt.is_done():
        assert tt.curr().type == misc.TokenType.OPER
        op = tt.curr().value
        (op_prec, op_assoc) = paxe.precedence_climbing_infix(op)
        if op_prec < curr_prec:
            break
        tt.token_idx += 1
        used_prec = op_prec + (1 if op_assoc == parse_axe.Assoc.LEFT_TO_RIGHT else 0)
        rhs = expr_impl(paxe, tt, used_prec)
        result = misc.cons_str(op, result, rhs)

    return result


def precedence_climbing(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]):
    tt = misc.Tokenization(tokens)
    return expr_impl(paxe, tt, 0)


def _run():
    with testset.Testset(precedence_climbing) as ts:
        ts.infix_only()
        # ts.allfix()
        # ts.parentheses()
        # ts.subscript()
        # ts.ternary()


if __name__ == '__main__':
    _run()
