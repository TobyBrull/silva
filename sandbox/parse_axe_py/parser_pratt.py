#!/usr/bin/env python
import misc
import testset
import parse_axe


def expr_impl(paxe: parse_axe.ParseAxe, tt: misc.Tokenization, min_prec: int) -> str:
    x = tt.curr()
    tt.token_idx += 1
    if x.type == misc.TokenType.ATOM:
        lhs = x.value
    elif (x.type == misc.TokenType.OPER) and (x.value == paxe.transparent_left_bracket()):
        lhs = expr_impl(paxe, tt, 0)
        assert tt.curr().value == paxe.transparent_right_bracket()
        tt.token_idx += 1
    elif x.type == misc.TokenType.OPER:
        prec = paxe.pratt_prefix(x.value)
        assert prec is not None
        assert prec >= min_prec, f'precedence order mismatch'
        rhs = expr_impl(paxe, tt, prec)
        lhs = misc.cons_str(x.value, rhs)
    else:
        raise RuntimeError(f"bad token: {x}")

    postfix_prec = parse_axe.BINDING_POWER_INF_RIGHT
    while True:
        if tt.is_done():
            break

        match tt.curr():
            case misc.Token(misc.TokenType.OPER, op):
                pass
            case t:
                raise RuntimeError(f"bad token: {t}")

        if (prec := paxe.pratt_postfix(op)) is not None:
            assert prec <= postfix_prec, f'precedence order mismatch'
            postfix_prec = prec
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

        else:
            if (res := paxe.pratt_infix(op)) is not None:
                (left_prec, right_prec) = res
                assert right_prec <= postfix_prec, f'precedence order mismatch'
                if left_prec < min_prec:
                    break
                tt.token_idx += 1

                rhs = expr_impl(paxe, tt, right_prec)
                lhs = misc.cons_str(op, lhs, rhs)

            elif (res := paxe.pratt_ternary(op)) is not None:
                (prec, second_op) = res
                assert prec <= postfix_prec, f'precedence order mismatch'
                if prec < min_prec:
                    break
                tt.token_idx += 1

                mhs = expr_impl(paxe, tt, prec)
                assert tt.curr().value == second_op
                tt.token_idx += 1
                rhs = expr_impl(paxe, tt, prec)
                lhs = misc.cons_str(op, lhs, mhs, rhs)

            else:
                break

            postfix_prec = parse_axe.BINDING_POWER_INF_RIGHT

    return lhs


def pratt(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    tt = misc.Tokenization(tokens)
    return expr_impl(paxe, tt, 0)


def _run():
    with testset.Testset(pratt) as ts:
        ts.infix_only()
        ts.allfix()
        ts.parentheses()
        # ts.subscript()
        ts.ternary()


if __name__ == '__main__':
    _run()
