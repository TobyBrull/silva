#!/usr/bin/env python
import misc
import testset
import parse_axe


def expr_impl(
    paxe: parse_axe.ParseAxe, tokens: list[misc.Token], index: int, min_prec: int
) -> tuple[str, int]:
    assert index < len(tokens)
    token = tokens[index]
    index += 1
    if token.type == misc.TokenType.ATOM:
        lhs = token.value
    elif (token.type == misc.TokenType.OPER) and (token.value == paxe.transparent_brackets[0]):
        lhs, index = expr_impl(paxe, tokens, index, 0)
        assert index < len(tokens)
        assert tokens[index].value == paxe.transparent_brackets[1]
        index += 1
    elif token.type == misc.TokenType.OPER:
        prec = paxe.prec_prefix(token.value)
        assert prec is not None, f'{token=}'
        assert prec >= min_prec, f'precedence order mismatch'
        rhs, index = expr_impl(paxe, tokens, index, prec)
        lhs = misc.cons_str(token.value, rhs)
    else:
        raise RuntimeError(f"bad token: {token}")

    postfix_prec = parse_axe.BINDING_POWER_INF_RIGHT
    while index < len(tokens):
        assert tokens[index].type == misc.TokenType.OPER
        op_name = tokens[index].value

        if (prec := paxe.prec_postfix(op_name)) is not None:
            assert prec <= postfix_prec, f'precedence order mismatch'
            postfix_prec = prec
            if prec < min_prec:
                break
            index += 1
            lhs = misc.cons_str(op_name, lhs)

        elif (res := paxe.prec_postfix_bracketed(op_name)) is not None:
            (prec, closing_bracket_name) = res
            assert prec <= postfix_prec, f'precedence order mismatch'
            postfix_prec = prec
            if prec < min_prec:
                break
            index += 1

            rhs, index = expr_impl(paxe, tokens, index, 0)
            lhs = misc.cons_str(op_name, lhs, rhs)
            assert index < len(tokens)
            assert tokens[index].value == closing_bracket_name
            index += 1

        else:
            if (res := paxe.prec_infix(op_name)) is not None:
                (left_prec, right_prec) = res
                assert right_prec <= postfix_prec, f'precedence order mismatch'
                if left_prec < min_prec:
                    break
                index += 1

                rhs, index = expr_impl(paxe, tokens, index, right_prec)
                lhs = misc.cons_str(op_name, lhs, rhs)

            elif (res := paxe.prec_ternary(op_name)) is not None:
                (prec, second_op) = res
                assert prec <= postfix_prec, f'precedence order mismatch'
                if prec < min_prec:
                    break
                index += 1

                mhs, index = expr_impl(paxe, tokens, index, 0)
                assert index < len(tokens)
                assert tokens[index].value == second_op
                index += 1
                rhs, index = expr_impl(paxe, tokens, index, prec)
                lhs = misc.cons_str(op_name, lhs, mhs, rhs)

            else:
                break

            postfix_prec = parse_axe.BINDING_POWER_INF_RIGHT

    return lhs, index


def pratt(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    retval, index = expr_impl(paxe, tokens, 0, 0)
    assert index == len(tokens)
    return retval


def _run():
    testset.execute(pratt)


if __name__ == '__main__':
    _run()
