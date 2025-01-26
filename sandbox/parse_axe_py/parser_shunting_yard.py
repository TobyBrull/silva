#!/usr/bin/env python
import dataclasses

import misc
import testset
import parse_axe


@dataclasses.dataclass(slots=True)
class Frame:
    min_bp: int
    lhs: str | None
    token: str | None


def expr_impl(paxe: parse_axe.ParseAxe, tt: misc.Tokenization) -> str:
    stack: list[Frame] = [Frame(0, None, None)]

    def stack_pop():
        popped = stack.pop()
        stack[-1].lhs = misc.cons_str(popped.token, stack[-1].lhs, popped.lhs)

    for token_t in tt.tokens:
        token = token_t.value if token_t.value else None
        while True:
            (l_bp, r_bp) = binding_power(paxe, token_t, stack[-1].lhs is None)
            if stack[-1].min_bp <= l_bp:
                break
            else:
                assert len(stack) > 1
                stack_pop()

        if token == ')':
            assert stack[-1].token == '('
            popped = stack.pop()
            stack[-1].lhs = popped.lhs
            continue

        stack.append(Frame(min_bp=r_bp, lhs=None, token=token))

    while len(stack) > 1:
        stack_pop()
    assert stack[0].lhs
    return stack[0].lhs


def binding_power(paxe: parse_axe.ParseAxe, token: misc.Token, prefix: bool) -> tuple[int, int]:
    if token.type == misc.TokenType.ATOM:
        return parse_axe.BINDING_POWER_INF_LEFT, parse_axe.BINDING_POWER_INF_RIGHT
    elif (token.type == misc.TokenType.OPER) and (token.value == paxe.transparent_left_bracket()):
        return parse_axe.BINDING_POWER_INF_LEFT, 0
    elif (token.type == misc.TokenType.OPER) and (token.value == paxe.transparent_right_bracket()):
        return 0, parse_axe.BINDING_POWER_INF_RIGHT
    else:
        return paxe.shuting_yard_prec(token.value, prefix)


def shunting_yard(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    tt = misc.Tokenization(tokens)
    return expr_impl(paxe, tt)


def _run():
    with testset.Testset(shunting_yard) as ts:
        ts.infix_only()
        # ts.allfix()
        ts.parentheses()
        # ts.subscript()
        # ts.ternary()


if __name__ == '__main__':
    _run()
