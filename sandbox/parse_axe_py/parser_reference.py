#!/usr/bin/env python
import dataclasses
import enum
import pprint

import misc
import testset
import parse_axe


class RefTokenType(enum.Enum):
    PRIMARY = 0
    PREFIX = 1
    POSTFIX = 2
    INFIX = 3
    TERNARY_OPEN = 4
    TERNARY_CLOSE = 4


@dataclasses.dataclass
class RefToken:
    type: RefTokenType
    value: str


def _to_ref_tokens(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> list[RefToken]:
    retval = []
    postfix_mode = False
    for token in tokens:
        if token.type == misc.TokenType.ATOM:
            retval.append(RefToken(type=RefTokenType.PRIMARY, value=token.value))
            postfix_mode = True
        elif token.type == misc.TokenType.OPER:
            ome = paxe.op_map[token.value]
            if ome.postfix_index is not None:
                assert postfix_mode
                retval.append(RefToken(type=RefTokenType.POSTFIX, value=token.value))
            elif ome.ternary_index is not None:
                level = paxe.levels[ome.ternary_index]
                assert type(level) == parse_axe.LevelTernary
                assert postfix_mode
                if token.value == level.first_op:
                    retval.append(RefToken(type=RefTokenType.TERNARY_OPEN, value=token.value))
                    postfix_mode = False
                elif token.value == level.second_op:
                    retval.append(RefToken(type=RefTokenType.TERNARY_CLOSE, value=token.value))
                    postfix_mode = False
                else:
                    raise Exception(f'Unkonwn {token.value=}')
            elif (ome.prefix_index is not None) and (ome.infix_index is not None):
                if postfix_mode:
                    retval.append(RefToken(type=RefTokenType.INFIX, value=token.value))
                    postfix_mode = False
                else:
                    retval.append(RefToken(type=RefTokenType.PREFIX, value=token.value))
            elif ome.prefix_index is not None:
                assert not postfix_mode
                retval.append(RefToken(type=RefTokenType.PREFIX, value=token.value))
            elif ome.infix_index is not None:
                assert postfix_mode
                retval.append(RefToken(type=RefTokenType.INFIX, value=token.value))
                postfix_mode = False
            else:
                raise Exception(f'Unknown {ome=}')
        else:
            raise Exception(f'Unknown {token.type=}')
    return retval


def _reduce(assoc: parse_axe.Assoc, window: int, ref_tokens: list[RefToken], f) -> list[RefToken]:
    while True:
        rr = list(range(0, len(ref_tokens) - window + 1))
        if assoc == parse_axe.Assoc.RIGHT_TO_LEFT:
            rr = reversed(rr)
        changed = False
        for i in rr:
            subwnd = ref_tokens[i : i + window]
            result = f(subwnd)
            if result is not None:
                changed = True
                ref_tokens = (
                    ref_tokens[:i]
                    + [RefToken(type=RefTokenType.PRIMARY, value=result)]
                    + ref_tokens[i + window :]
                )
                break
        if not changed:
            break
    return ref_tokens


def _reduce_ternary_impl(
    ref_tokens: list[RefToken],
    first_op: str,
    second_op: str,
    index: int,
) -> tuple[str, int, int]:
    assert 1 <= index and index + 3 < len(ref_tokens)
    assert ref_tokens[index - 1].type == RefTokenType.PRIMARY
    assert ref_tokens[index].value == first_op
    assert ref_tokens[index + 1].type == RefTokenType.PRIMARY
    assert ref_tokens[index + 2].value in (first_op, second_op)
    assert ref_tokens[index + 3].type == RefTokenType.PRIMARY
    if ref_tokens[index + 2].value == first_op:
        sub_result, sub_begin, sub_end = _reduce_ternary_impl(
            ref_tokens, first_op, second_op, index + 2
        )
        assert sub_begin == index + 1
        assert sub_end + 1 < len(ref_tokens)
        assert ref_tokens[sub_end].value == second_op
        assert ref_tokens[sub_end + 1].type == RefTokenType.PRIMARY
        return (
            misc.cons_str(
                first_op, ref_tokens[index - 1].value, sub_result, ref_tokens[sub_end + 1].value
            ),
            index - 1,
            sub_end + 2,
        )
    elif ref_tokens[index + 2].value == second_op:
        if index + 4 < len(ref_tokens) and ref_tokens[index + 4].value == first_op:
            sub_result, sub_begin, sub_end = _reduce_ternary_impl(
                ref_tokens, first_op, second_op, index + 4
            )
        else:
            sub_result, sub_begin, sub_end = (ref_tokens[index + 3].value, index + 3, index + 4)
        return (
            misc.cons_str(
                first_op, ref_tokens[index - 1].value, ref_tokens[index + 1].value, sub_result
            ),
            index - 1,
            sub_end,
        )
    else:
        raise Exception(f'Unexpected {ref_tokens[index+2]=}')


def _reduce_ternary(ref_tokens: list[RefToken], level: parse_axe.LevelTernary) -> list[RefToken]:
    first_op, second_op = level.first_op, level.second_op
    while True:
        changed = False
        for i in range(len(ref_tokens)):
            if ref_tokens[i].type == RefTokenType.TERNARY_OPEN and ref_tokens[i].value == first_op:
                primary_str, begin, end = _reduce_ternary_impl(ref_tokens, first_op, second_op, i)
                ref_tokens = (
                    ref_tokens[:begin]
                    + [RefToken(type=RefTokenType.PRIMARY, value=primary_str)]
                    + ref_tokens[end:]
                )
                changed = True
                break
        if not changed:
            break

    return ref_tokens


def reference(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    ref_tokens = _to_ref_tokens(paxe, tokens)
    for level in reversed(paxe.levels):
        if type(level) == parse_axe.LevelPrefix:
            ops = level.ops

            def _f(wnd: list[RefToken]):
                assert len(wnd) == 2
                if (
                    wnd[0].type == RefTokenType.PREFIX
                    and wnd[1].type == RefTokenType.PRIMARY
                    and wnd[0].value in ops
                ):
                    return misc.cons_str(wnd[0].value, wnd[1].value)
                return None

            ref_tokens = _reduce(parse_axe.Assoc.RIGHT_TO_LEFT, 2, ref_tokens, _f)
        elif type(level) == parse_axe.LevelInfix:
            ops = level.ops

            def _f(wnd: list[RefToken]):
                assert len(wnd) == 3
                if (
                    wnd[0].type == RefTokenType.PRIMARY
                    and wnd[1].type == RefTokenType.INFIX
                    and wnd[2].type == RefTokenType.PRIMARY
                    and wnd[1].value in ops
                ):
                    return misc.cons_str(wnd[1].value, wnd[0].value, wnd[2].value)
                return None

            ref_tokens = _reduce(level.assoc, 3, ref_tokens, _f)
        elif type(level) == parse_axe.LevelPostfix:
            ops = level.ops

            def _f(wnd: list[RefToken]):
                assert len(wnd) == 2
                if (
                    wnd[0].type == RefTokenType.PRIMARY
                    and wnd[1].type == RefTokenType.POSTFIX
                    and wnd[1].value in ops
                ):
                    return misc.cons_str(wnd[1].value, wnd[0].value)
                return None

            ref_tokens = _reduce(parse_axe.Assoc.LEFT_TO_RIGHT, 2, ref_tokens, _f)
        elif type(level) == parse_axe.LevelPostfixExpr:
            pass
        elif type(level) == parse_axe.LevelTernary:
            ref_tokens = _reduce_ternary(ref_tokens, level)
        else:
            raise Exception(f'Unknown {level=}')
    assert len(ref_tokens) == 1 and ref_tokens[0].type == RefTokenType.PRIMARY, pprint.pformat(
        ref_tokens
    )
    return ref_tokens[0].value


def _run():
    with testset.Testset(reference) as ts:
        pass
        ts.infix_only()
        ts.allfix()
        # ts.parentheses()
        # ts.subscript()
        ts.ternary()


if __name__ == '__main__':
    _run()
