import misc
import parse_axe
import testset


def expr_impl(paxe: parse_axe.ParseAxe, tt: misc.Tokenization, curr_prec: int) -> str:
    assert tt.curr().type == misc.TokenType.ATOM
    result = tt.curr().value
    tt.token_idx += 1

    while not tt.is_done():
        assert tt.curr().type == misc.TokenType.OP
        op = tt.curr().value
        (op_prec, op_assoc) = paxe.infix_prec(op)
        if op_prec < curr_prec:
            break
        tt.token_idx += 1
        used_prec = op_prec + (1 if op_assoc == parse_axe.Assoc.LEFT_TO_RIGHT else 0)
        rhs = expr_impl(paxe, tt, used_prec)
        result = misc.cons_str(op, result, rhs)

    return result


def precedence_climbing(paxe: parse_axe.ParseAxe, tt: misc.Tokenization):
    return expr_impl(paxe, tt, 0)


if __name__ == "__main__":
    ts = testset.Testset(precedence_climbing)
    ts.infix_only()
    # ts.allfix()
    # ts.parentheses()
    # ts.subscript()
    # ts.ternary()
