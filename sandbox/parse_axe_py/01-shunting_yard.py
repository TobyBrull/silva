import dataclasses

import misc
import expr_tests
import parse_axe


@dataclasses.dataclass(slots=True)
class Frame:
    min_bp: int
    lhs: str | None
    token: str | None


def expr_impl(tt: misc.Tokenization):
    stack: list[Frame] = [Frame(0, None, None)]

    def stack_pop():
        popped = stack.pop()
        stack[-1].lhs = misc.cons_str(popped.token, stack[-1].lhs, popped.lhs)

    for token_t in tt.tokens:
        token = token_t.value if token_t.value else None
        while True:
            (l_bp, r_bp) = binding_power(token_t, stack[-1].lhs is None)
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
    return stack[0].lhs


def binding_power(token: misc.Token, prefix: bool) -> tuple[int, int]:
    if token.type == misc.TokenType.ATOM:
        return 99, 100
    else:
        match token.value:
            case '(':
                return 99, 0
            case ')':
                return 0, 100
            case '=':
                return 2, 1
            case '+' | '-' if prefix:
                return 99, 9
            case '+' | '-':
                return 5, 6
            case '*' | '/':
                return 7, 8
            case '!':
                return 11, 100
            case '.':
                return 14, 13
            case _:
                assert False, f'Unexpected {token}'


def shunting_yard(paxe: parse_axe.ParseAxe, tt: misc.Tokenization):
    return expr_impl(tt)


if __name__ == '__main__':
    expr_tests.all(shunting_yard)
