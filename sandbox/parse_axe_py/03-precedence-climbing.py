import misc
import parse_axe
import expr_tests


def precedence_climbing(tt: misc.Tokenization):
    pa = parse_axe.default_parse_axe()
    return expr_impl(pa, tt, 0)


def expr_impl(pa: parse_axe.ParseAxe, tt: misc.Tokenization, curr_prec: int) -> str:
    assert tt.curr().type == misc.TokenType.ATOM
    result = tt.curr().value
    tt.token_idx += 1

    while not tt.is_done():
        assert tt.curr().type == misc.TokenType.OP
        op = tt.curr().value
        (op_prec, op_assoc) = pa.infix_prec(op)
        if op_prec < curr_prec:
            break
        tt.token_idx += 1
        used_prec = op_prec + (1 if op_assoc == parse_axe.PrecLevelType.INFIX_LTR else 0)
        rhs = expr_impl(pa, tt, used_prec)
        result = misc.cons_str(op, result, rhs)

    return result


if __name__ == "__main__":
    expr_tests.all3(precedence_climbing)
