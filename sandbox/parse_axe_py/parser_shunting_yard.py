#!/usr/bin/env python
import dataclasses
import pprint

import misc
import testset
import parse_axe


@dataclasses.dataclass(slots=True)
class OperStackEntry:
    op: parse_axe.Op
    right_prec: int
    level_info: parse_axe.LevelInfo
    arity: int
    token_indexes: list[int]
    min_token_index: int | None = None
    max_token_index: int | None = None


@dataclasses.dataclass(slots=True)
class AtomStackEntry:
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


def expr_impl(paxe: parse_axe.ParseAxe, tokens: list[misc.Token], begin: int) -> AtomStackEntry:
    oper_stack: list[OperStackEntry] = []
    atom_stack: list[AtomStackEntry] = []
    prefix_mode = True
    index = begin

    def stack_pop(prec: int):
        nonlocal oper_stack, atom_stack
        while (len(oper_stack) >= 1) and oper_stack[-1].right_prec > prec:
            ose = oper_stack[-1]
            oper_stack.pop()
            new_atom_name = ose.level_info.name + ose.op.render(
                *[x.name for x in atom_stack[-ose.arity :]]
            )
            token_begin, token_end = _consistent_range(
                ose.token_indexes,
                [(x.token_begin, x.token_end) for x in atom_stack[-ose.arity :]],
            )
            atom_stack = atom_stack[: -ose.arity]
            assert (ose.min_token_index is None) or (ose.min_token_index <= token_begin)
            assert (ose.max_token_index is None) or (token_end <= ose.max_token_index)
            atom_stack.append(
                AtomStackEntry(
                    name=new_atom_name,
                    token_begin=token_begin,
                    token_end=token_end,
                )
            )

    def hallucinate_concat():
        nonlocal prefix_mode, index
        level_info = paxe.lookup_concat()
        assert level_info is not None
        left_prec, right_prec = level_info.left_and_right_prec()
        stack_pop(left_prec)
        oper_stack.append(
            OperStackEntry(
                op=parse_axe.Infix(None),
                right_prec=right_prec,
                level_info=level_info,
                arity=2,
                token_indexes=[],
            )
        )
        prefix_mode = True

    def handle_bracketed(left_bracket, right_bracket):
        nonlocal prefix_mode, index
        assert index < len(tokens)
        assert tokens[index].name == left_bracket
        retval = expr_impl(paxe, tokens, index + 1)
        assert retval.token_end < len(tokens)
        assert tokens[retval.token_end].name == right_bracket
        index = retval.token_end + 1
        retval.token_begin -= 1
        retval.token_end += 1
        return retval

    has_concat = paxe.lookup_concat() is not None

    while index < len(tokens):
        token = tokens[index]
        if token.type == misc.TokenType.ATOM:

            if prefix_mode:
                atom_stack.append(
                    AtomStackEntry(
                        name=token.name,
                        token_begin=index,
                        token_end=index + 1,
                    )
                )
                prefix_mode = False
                index += 1
                continue

            if not prefix_mode and has_concat:
                hallucinate_concat()
                continue

        elif tokens[index].type == misc.TokenType.OPER:

            if paxe.is_right_bracket(token.name):
                break

            flr = paxe.lookup(token.name)

            if not prefix_mode and has_concat:
                if flr.prefix_res is not None and flr.regular_res is None:
                    hallucinate_concat()
                    continue

                if flr.transparent_brackets():
                    hallucinate_concat()
                    continue

            if prefix_mode and (res := flr.transparent_brackets()) is not None:
                atom = handle_bracketed(res[0], res[1])
                atom_stack.append(atom)
                prefix_mode = False
                continue

            if prefix_mode:
                assert flr.prefix_res is not None
                (op, level_info) = flr.prefix_res
                stack_pop(level_info.prec)

                if type(op) == parse_axe.Prefix:
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=level_info.prec,
                            level_info=level_info,
                            arity=1,
                            token_indexes=[index],
                            min_token_index=index,
                        )
                    )
                    index += 1
                    continue

                if type(op) == parse_axe.PrefixBracketed:
                    atom = handle_bracketed(op.left_bracket, op.right_bracket)
                    atom_stack.append(atom)
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=level_info.prec,
                            level_info=level_info,
                            arity=2,
                            token_indexes=[],
                            min_token_index=atom.token_begin,
                        )
                    )
                    continue

            else:
                assert flr.regular_res is not None
                (op, level_info) = flr.regular_res
                left_prec, right_prec = level_info.left_and_right_prec()
                stack_pop(left_prec)

                if type(op) == parse_axe.Postfix:
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=right_prec,
                            level_info=level_info,
                            arity=1,
                            token_indexes=[index],
                            max_token_index=index + 1,
                        )
                    )
                    index += 1
                    continue

                if type(op) == parse_axe.PostfixBracketed:
                    atom = handle_bracketed(op.left_bracket, op.right_bracket)
                    atom_stack.append(atom)
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=right_prec,
                            level_info=level_info,
                            arity=2,
                            token_indexes=[],
                            max_token_index=atom.token_end,
                        )
                    )
                    continue

                if type(op) == parse_axe.Infix:
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=right_prec,
                            level_info=level_info,
                            arity=2,
                            token_indexes=[index],
                        )
                    )
                    prefix_mode = True
                    index += 1
                    continue

                if type(op) == parse_axe.Ternary:
                    atom_mid = handle_bracketed(op.first_name, op.second_name)
                    atom_stack.append(atom_mid)
                    oper_stack.append(
                        OperStackEntry(
                            op=op,
                            right_prec=right_prec,
                            level_info=level_info,
                            arity=3,
                            token_indexes=[],
                        )
                    )
                    prefix_mode = True
                    continue

        raise Exception(f'Unknown {tokens[index]=}')

    stack_pop(0)
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
