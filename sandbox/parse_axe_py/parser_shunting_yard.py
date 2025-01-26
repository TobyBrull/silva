#!/usr/bin/env python
import dataclasses
import pprint

import misc
import testset
import parse_axe


@dataclasses.dataclass(slots=True)
class OperStackEntry:
    name: str
    prec: int
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

    def stack_pop(prec: int):
        nonlocal atom_stack
        # print(f'push to {prec=}')
        # pprint.pprint(atom_stack)
        while (len(oper_stack) >= 1) and oper_stack[-1].prec > prec:
            ose = oper_stack[-1]
            new_atom_name = misc.cons_str(ose.name, *[x.name for x in atom_stack[-ose.arity :]])
            token_begin, token_end = _consistent_range(
                ose.token_indexes,
                [(x.token_begin, x.token_end) for x in atom_stack[-ose.arity :]],
            )
            assert (ose.min_token_index is None) or (ose.min_token_index <= token_begin)
            assert (ose.max_token_index is None) or (token_end <= ose.max_token_index)
            new_atom = AtomStackEntry(
                name=new_atom_name, token_begin=token_begin, token_end=token_end
            )
            atom_stack = atom_stack[: -ose.arity]
            atom_stack.append(new_atom)
            oper_stack.pop()

    prefix_mode = True
    index = begin
    while index < len(tokens):
        token_index = index
        token = tokens[index]
        is_atomar = (token.type == misc.TokenType.ATOM) or (
            prefix_mode
            and token.type == misc.TokenType.OPER
            and token.value == paxe.transparent_brackets[0]
        )
        if is_atomar:
            if token.type == misc.TokenType.ATOM:
                atom = AtomStackEntry(name=token.value, token_begin=index, token_end=index + 1)
            else:
                assert (
                    prefix_mode
                    and token.type == misc.TokenType.OPER
                    and token.value == paxe.transparent_brackets[0]
                )
                atom = expr_impl(paxe, tokens, index + 1)
                assert (
                    atom.token_end < len(tokens)
                    and tokens[atom.token_end].value == paxe.transparent_brackets[1]
                )
                index = atom.token_end
                atom.token_begin -= 1
                atom.token_end += 1
            atom_stack.append(atom)
            prefix_mode = False
        else:
            assert token.type == misc.TokenType.OPER
            oper_name = token.value
            if prefix_mode:
                prec = paxe.pratt_prefix(oper_name)
                assert prec
                stack_pop(prec)
                oper = OperStackEntry(
                    name=oper_name,
                    prec=prec,
                    arity=1,
                    token_indexes=[token_index],
                    min_token_index=token_index,
                )
            else:
                if paxe.is_right_bracket(oper_name):
                    break
                elif res := paxe.pratt_postfix(oper_name):
                    (prec, right_bracket) = res
                    stack_pop(prec)
                    token_indexes = []
                    if right_bracket is None:
                        arity = 1
                        token_indexes.append(token_index)
                        max_token_index = token_index + 1
                    else:
                        sub_atom = expr_impl(paxe, tokens, index + 1)
                        assert sub_atom.token_end < len(tokens)
                        assert tokens[sub_atom.token_end].value == right_bracket
                        index = sub_atom.token_end
                        sub_atom.token_begin -= 1
                        sub_atom.token_end += 1
                        max_token_index = sub_atom.token_end
                        atom_stack.append(sub_atom)
                        arity = 2
                        prec += 1 # PostfixBracketed is always left-to-right
                    oper = OperStackEntry(
                        name=oper_name,
                        prec=prec,
                        arity=arity,
                        token_indexes=token_indexes,
                        max_token_index=max_token_index,
                    )
                elif res := paxe.pratt_infix(oper_name):
                    (left_prec, right_prec) = res
                    stack_pop(left_prec)
                    oper = OperStackEntry(
                        name=oper_name,
                        prec=right_prec,
                        arity=2,
                        token_indexes=[token_index],
                    )
                    prefix_mode = True
                else:
                    raise Exception(f'Unknown {oper_name=}')
            oper_stack.append(oper)
        index += 1

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
    testset.execute(
        shunting_yard,
        excluded=[
            'base/ternary',
        ],
    )


if __name__ == '__main__':
    _run()
