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
        while (len(oper_stack) >= 1) and oper_stack[-1].prec > prec:
            ose = oper_stack[-1]
            oper_stack.pop()
            new_atom_name = misc.cons_str(ose.name, *[x.name for x in atom_stack[-ose.arity :]])
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

    prefix_mode = True
    index = begin
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

            if not prefix_mode and (res := paxe.prec_infix(None)):
                (left_prec, right_prec) = res
                stack_pop(left_prec)
                atom_stack.append(
                    AtomStackEntry(
                        name=token.name,
                        token_begin=index,
                        token_end=index + 1,
                    )
                )
                oper_stack.append(
                    OperStackEntry(
                        name='Concat',
                        prec=right_prec,
                        arity=2,
                        token_indexes=[],
                    )
                )
                index += 1
                continue

        elif tokens[index].type == misc.TokenType.OPER:

            flr = paxe.lookup(token.name)

            if not prefix_mode and paxe.prec_prefix(token.name) and (res := paxe.prec_infix(None)):
                (left_prec, right_prec) = res
                stack_pop(left_prec)
                oper_stack.append(
                    OperStackEntry(
                        name='Concat',
                        prec=right_prec,
                        arity=2,
                        token_indexes=[],
                    )
                )
                prefix_mode = True
                continue

            if prefix_mode and token.name == paxe.transparent_brackets[0]:
                atom = expr_impl(paxe, tokens, index + 1)
                assert (
                    atom.token_end < len(tokens)
                    and tokens[atom.token_end].name == paxe.transparent_brackets[1]
                )
                index = atom.token_end
                atom.token_begin -= 1
                atom.token_end += 1
                atom_stack.append(atom)
                prefix_mode = False
                index += 1
                continue

            if not prefix_mode and paxe.is_right_bracket(token.name):
                break

            if prefix_mode:
                assert flr.prefix_res is not None
                (op, prec, assoc) = flr.prefix_res

                if type(op) == parse_axe.Prefix:
                    stack_pop(prec)
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=prec,
                            arity=1,
                            token_indexes=[index],
                            min_token_index=index,
                        )
                    )
                    index += 1
                    continue

                if type(op) == parse_axe.PrefixBracketed:
                    stack_pop(prec)
                    sub_atom = expr_impl(paxe, tokens, index + 1)
                    assert sub_atom.token_end < len(tokens)
                    assert tokens[sub_atom.token_end].name == op.right_bracket
                    index = sub_atom.token_end
                    sub_atom.token_begin -= 1
                    sub_atom.token_end += 1
                    min_token_index = sub_atom.token_begin
                    atom_stack.append(sub_atom)
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=prec,
                            arity=2,
                            token_indexes=[],
                            min_token_index=min_token_index,
                        )
                    )
                    index += 1
                    continue

            else:
                assert flr.regular_res is not None
                (op, prec, assoc) = flr.regular_res

                if type(op) == parse_axe.Postfix:
                    stack_pop(prec)
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=prec,
                            arity=1,
                            token_indexes=[index],
                            max_token_index=index + 1,
                        )
                    )
                    index += 1
                    continue

                if type(op) == parse_axe.PostfixBracketed:
                    stack_pop(prec)
                    sub_atom = expr_impl(paxe, tokens, index + 1)
                    assert sub_atom.token_end < len(tokens)
                    assert tokens[sub_atom.token_end].name == op.right_bracket
                    index = sub_atom.token_end
                    sub_atom.token_begin -= 1
                    sub_atom.token_end += 1
                    max_token_index = sub_atom.token_end
                    atom_stack.append(sub_atom)
                    prec += 1  # PostfixBracketed is always left-to-right
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=prec,
                            arity=2,
                            token_indexes=[],
                            max_token_index=max_token_index,
                        )
                    )
                    index += 1
                    continue

                if type(op) == parse_axe.Infix:
                    (left_prec, right_prec) = (
                        (prec, prec + 1)
                        if (assoc == parse_axe.Assoc.LEFT_TO_RIGHT)
                        else (prec + 1, prec)
                    )
                    stack_pop(left_prec)
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=right_prec,
                            arity=2,
                            token_indexes=[index],
                        )
                    )
                    prefix_mode = True
                    index += 1
                    continue

                if type(op) == parse_axe.Ternary:
                    stack_pop(prec)
                    sub_atom_mid = expr_impl(paxe, tokens, index + 1)
                    assert sub_atom_mid.token_end < len(tokens)
                    assert tokens[sub_atom_mid.token_end].name == op.second_name
                    first_token_index = index
                    second_token_index = sub_atom_mid.token_end
                    index = sub_atom_mid.token_end
                    atom_stack.append(sub_atom_mid)
                    oper_stack.append(
                        OperStackEntry(
                            name=token.name,
                            prec=prec,
                            arity=3,
                            token_indexes=[first_token_index, second_token_index],
                        )
                    )
                    prefix_mode = True
                    index += 1
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
