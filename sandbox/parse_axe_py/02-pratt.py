import misc
import testset
import parse_axe


def expr_impl(paxe: parse_axe.ParseAxe, tt: misc.Tokenization, min_prec: int):
    x = tt.curr()
    tt.token_idx += 1
    match x:
        case misc.Token(misc.TokenType.ATOM, it):
            lhs = it
        case misc.Token(misc.TokenType.OP, '('):
            lhs = expr_impl(paxe, tt, 0)
            assert tt.curr().value == ')'
            tt.token_idx += 1
        case misc.Token(misc.TokenType.OP, op):
            prec = paxe.pratt_prefix(op)
            assert prec is not None
            rhs = expr_impl(paxe, tt, prec)
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

        if (prec := paxe.pratt_postfix(op)) is not None:
            if prec < min_prec:
                break
            tt.curr()
            tt.token_idx += 1

            if op == '[':
                rhs = expr_impl(paxe, tt, 0)
                lhs = misc.cons_str(op, lhs, rhs)
                assert tt.curr().value == ']'
                tt.token_idx += 1
            else:
                lhs = misc.cons_str(op, lhs)

        elif (res := paxe.pratt_infix(op)) is not None:
            (left_prec, right_prec) = res
            if left_prec < min_prec:
                break
            tt.token_idx += 1

            rhs = expr_impl(paxe, tt, right_prec)
            lhs = misc.cons_str(op, lhs, rhs)

        elif (res := paxe.pratt_ternary(op)) is not None:
            (prec, second_op) = res
            if prec < min_prec:
                break
            tt.token_idx += 1

            mhs = expr_impl(paxe, tt, 0)
            assert tt.curr().value == second_op
            tt.token_idx += 1
            rhs = expr_impl(paxe, tt, prec)
            lhs = misc.cons_str(op, lhs, mhs, rhs)

        else:
            break

    return lhs


def pratt(paxe: parse_axe.ParseAxe, tt: misc.Tokenization):
    return expr_impl(paxe, tt, 0)


if __name__ == "__main__":
    ts = testset.Testset(pratt)
    ts.infix_only()
    ts.allfix()
    ts.parentheses()
    ts.subscript()
    ts.ternary()
