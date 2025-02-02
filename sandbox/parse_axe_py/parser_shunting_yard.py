#!/usr/bin/env python
import dataclasses
import pprint
import enum

import misc
import testset
import parse_axe


@dataclasses.dataclass(slots=True)
class OperItem:
    op: parse_axe.Op
    level_info: parse_axe.LevelInfo
    token_indexes: list[int]
    min_token_index: int | None = None
    max_token_index: int | None = None


@dataclasses.dataclass(slots=True)
class AtomItem:
    name: str
    token_begin: int
    token_end: int


def _consistent_range(tokens: list[int], token_ranges: list[tuple[int, int]]) -> tuple[int, int]:
    all_token_ranges = []
    for token in tokens:
        all_token_ranges.append((token, token + 1))
    for token_range in token_ranges:
        all_token_ranges.append(token_range)
    all_token_ranges = sorted(all_token_ranges)
    for i in range(len(all_token_ranges) - 1):
        assert all_token_ranges[i][1] == all_token_ranges[i + 1][0]
    return all_token_ranges[0][0], all_token_ranges[-1][1]


class ShuntingYardMode(enum.Enum):

    # In the mode where an atom is naturally expected.
    # In this mode, a prefix operator may also appear next.
    ATOM = 1

    # In the mode where an infix operator is naturally expected.
    # In this mode, a postfix operator may also appear next.
    # Also, if concatenation is allowed, an atom or prefix may appear next, in which case a CONCAT
    # operator is hallucinated.
    INFIX = 2


ATOM_MODE = ShuntingYardMode.ATOM
INFIX_MODE = ShuntingYardMode.INFIX


def expr_impl(paxe: parse_axe.ParseAxe, tokens: list[misc.Token], begin: int) -> AtomItem:
    oper_stack: list[OperItem] = []
    atom_stack: list[AtomItem] = []
    mode = ATOM_MODE
    index = begin

    def should_apply(stack_level: parse_axe.LevelInfo, new_level: parse_axe.LevelInfo) -> bool:
        if stack_level.prec > new_level.prec:
            return True
        elif stack_level.prec < new_level.prec:
            return False
        else:
            assert stack_level.assoc == new_level.assoc
            assoc = stack_level.assoc
            return not (assoc == parse_axe.Assoc.RIGHT_TO_LEFT)

    def stack_pop(level_info: parse_axe.LevelInfo | None):
        nonlocal oper_stack, atom_stack

        while (len(oper_stack) >= 1) and (
            level_info is None or should_apply(oper_stack[-1].level_info, level_info)
        ):
            ose = oper_stack[-1]
            oper_stack.pop()
            new_atom_name = ose.level_info.name + ose.op.render(
                *[x.name for x in atom_stack[-ose.op.arity :]]
            )
            token_begin, token_end = _consistent_range(
                ose.token_indexes,
                [(x.token_begin, x.token_end) for x in atom_stack[-ose.op.arity :]],
            )
            atom_stack = atom_stack[: -ose.op.arity]
            assert (ose.min_token_index is None) or (ose.min_token_index <= token_begin)
            assert (ose.max_token_index is None) or (token_end <= ose.max_token_index)
            atom_stack.append(AtomItem(new_atom_name, token_begin, token_end))

    def hallucinate_concat():
        nonlocal mode, index
        level_info = paxe.get_concat_info()
        assert level_info is not None
        stack_pop(level_info)
        oper_stack.append(OperItem(parse_axe.Infix(None), level_info, []))
        mode = ATOM_MODE

    def handle_bracketed(left_bracket, right_bracket):
        nonlocal index
        assert index < len(tokens)
        assert tokens[index].name == left_bracket
        retval = expr_impl(paxe, tokens, index + 1)
        assert retval.token_end < len(tokens)
        assert tokens[retval.token_end].name == right_bracket
        index = retval.token_end + 1
        retval.token_begin -= 1
        retval.token_end += 1
        return retval

    while index < len(tokens):
        token = tokens[index]
        if token.type == misc.TokenType.ATOM:

            if mode == ATOM_MODE:
                atom_stack.append(AtomItem(token.name, index, index + 1))
                mode = INFIX_MODE
                index += 1
                continue

            if mode == INFIX_MODE and paxe.has_concat():
                hallucinate_concat()
                continue

        elif tokens[index].type == misc.TokenType.OPER:

            lr = paxe.lookup(token.name)

            if lr.is_right_bracket:
                break

            if mode == INFIX_MODE and paxe.has_concat():
                if lr.prefix_res is not None and lr.regular_res is None:
                    hallucinate_concat()
                    continue

                if lr.transparent_brackets():
                    hallucinate_concat()
                    continue

            if mode == ATOM_MODE and (res := lr.transparent_brackets()) is not None:
                atom = handle_bracketed(res[0], res[1])
                atom_stack.append(atom)
                mode = INFIX_MODE
                continue

            if mode == ATOM_MODE:
                assert lr.prefix_res is not None
                (op, level_info) = lr.prefix_res
                stack_pop(level_info)

                if type(op) == parse_axe.Prefix:
                    oper_stack.append(OperItem(op, level_info, [index], min_token_index=index))
                    index += 1
                    continue

                if type(op) == parse_axe.PrefixBracketed:
                    atom = handle_bracketed(op.left_bracket, op.right_bracket)
                    atom_stack.append(atom)
                    oper_stack.append(
                        OperItem(op, level_info, [], min_token_index=atom.token_begin)
                    )
                    continue

            else:
                assert lr.regular_res is not None
                (op, level_info) = lr.regular_res
                stack_pop(level_info)

                if type(op) == parse_axe.Postfix:
                    oper_stack.append(OperItem(op, level_info, [index], max_token_index=index + 1))
                    index += 1
                    continue

                if type(op) == parse_axe.PostfixBracketed:
                    atom = handle_bracketed(op.left_bracket, op.right_bracket)
                    atom_stack.append(atom)
                    oper_stack.append(OperItem(op, level_info, [], max_token_index=atom.token_end))
                    continue

                if type(op) == parse_axe.Infix:
                    oper_stack.append(OperItem(op, level_info, [index]))
                    mode = ATOM_MODE
                    index += 1
                    continue

                if type(op) == parse_axe.Ternary:
                    atom_mid = handle_bracketed(op.first_name, op.second_name)
                    atom_stack.append(atom_mid)
                    oper_stack.append(OperItem(op, level_info, []))
                    mode = ATOM_MODE
                    continue

        raise Exception(f'Unknown {tokens[index]=}')

    stack_pop(None)
    assert len(oper_stack) == 0, f'oper_stack not empty: {pprint.pformat(oper_stack)}'
    assert len(atom_stack) == 1, f'atom_stack not unit: {pprint.pformat(atom_stack)}'
    return atom_stack[0]


def shunting_yard(paxe: parse_axe.ParseAxe, tokens: list[misc.Token]) -> str:
    retval = expr_impl(paxe, tokens, 0)
    assert retval.token_begin == 0
    assert retval.token_end == len(tokens)
    return retval.name


def _run():
    testset.execute(shunting_yard)


if __name__ == '__main__':
    _run()
