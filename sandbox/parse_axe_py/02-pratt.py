import misc
import expr_tests
import parse_axe


def expr_impl(tt: misc.Tokenization, min_bp: int):
    x = tt.curr()
    tt.token_idx += 1
    match x:
        case misc.Token(misc.TokenType.ATOM, it):
            lhs = it
        case misc.Token(misc.TokenType.OP, '('):
            lhs = expr_impl(tt, 0)
            assert tt.curr().value == ')'
            tt.token_idx += 1
        case misc.Token(misc.TokenType.OP, op):
            _, r_bp = prefix_binding_power(op)
            rhs = expr_impl(tt, r_bp)
            lhs = misc.cons_str(op, rhs)
        case t:
            raise RuntimeError(f"bad token: {t}")

    while True:
        if tt.is_done():
            break
        match tt.curr():
            case misc.Token(misc.TokenType.OP, op):
                pass
            case t:
                raise RuntimeError(f"bad token: {t}")

        if pbp := postfix_binding_power(op):
            l_bp, _ = pbp
            if l_bp < min_bp:
                break
            tt.curr()
            tt.token_idx += 1

            if op == '[':
                rhs = expr_impl(tt, 0)
                lhs = misc.cons_str(op, lhs, rhs)
                assert tt.curr().value == ']'
                tt.token_idx += 1
            else:
                lhs = misc.cons_str(op, lhs)

        elif ibp := infix_binding_power(op):
            l_bp, r_bp = ibp
            if l_bp < min_bp:
                break
            tt.token_idx += 1

            if op == '?':
                mhs = expr_impl(tt, 0)
                assert tt.curr().value == ':'
                tt.token_idx += 1
                rhs = expr_impl(tt, r_bp)
                lhs = misc.cons_str(op, lhs, mhs, rhs)
            else:
                rhs = expr_impl(tt, r_bp)
                lhs = misc.cons_str(op, lhs, rhs)

        else:
            break

    return lhs


def prefix_binding_power(op: str) -> tuple[None, int]:
    bp = {
        '+': (None, 9),
        '-': (None, 9),
    }.get(op, None)
    if bp is not None:
        return bp
    else:
        raise RuntimeError(f"bad op: {op}")


def postfix_binding_power(op: str) -> tuple[int, None] | None:
    bp = {
        '!': (11, None),
        '[': (11, None),
    }.get(op, None)
    return bp


def infix_binding_power(op: str) -> tuple[int, int] | None:
    bp = {
        '=': (2, 1),
        '?': (4, 3),
        '+': (5, 6),
        '-': (5, 6),
        '*': (7, 8),
        '/': (7, 8),
        '.': (14, 13),
    }.get(op, None)
    return bp


def pratt(paxe: parse_axe.ParseAxe, tt: misc.Tokenization):
    return expr_impl(tt, 0)


if __name__ == "__main__":
    expr_tests.all2(pratt)
