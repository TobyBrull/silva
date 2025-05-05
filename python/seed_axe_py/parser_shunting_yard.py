#!/usr/bin/env python
import dataclasses
import pprint
import enum

import misc
import testset
import seed_axe
import parse_tree

Node = parse_tree.Node


@dataclasses.dataclass
class OperItem:
    op: seed_axe.Op
    level_info: seed_axe.LevelInfo
    token_indexes: list[int]
    min_token_index: int | None = None
    max_token_index: int | None = None


@dataclasses.dataclass
class AtomItem:
    node: Node
    flat_flag: bool
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


ATOM_MODE = seed_axe.ParseMode.ATOM
INFIX_MODE = seed_axe.ParseMode.INFIX


def expr_impl(saxe: seed_axe.SeedAxe, tokens: list[misc.Token], begin: int) -> AtomItem:
    oper_stack: list[OperItem] = []
    atom_stack: list[AtomItem] = []
    mode = ATOM_MODE
    index = begin

    def stack_pop(level_info: seed_axe.LevelInfo):
        nonlocal oper_stack, atom_stack
        while (len(oper_stack) >= 1) and not (oper_stack[-1].level_info < level_info):
            oi = oper_stack[-1]
            oper_stack.pop()
            token_begin, token_end = _consistent_range(
                oi.token_indexes,
                [(x.token_begin, x.token_end) for x in atom_stack[-oi.op.arity :]],
            )
            assert (oi.min_token_index is None) or (oi.min_token_index <= token_begin)
            assert (oi.max_token_index is None) or (token_end <= oi.max_token_index)
            if oi.level_info.assoc == seed_axe.Assoc.FLAT and atom_stack[-oi.op.arity].flat_flag:
                assert type(oi.op) == seed_axe.Infix
                assert oi.op.arity == 2
                base_node = atom_stack[-2].node
                add_node = atom_stack[-1].node
                if len(base_node.children) == 0:
                    new_node = Node(children=[base_node, Node(oi.op.name), add_node])
                else:
                    assert len(base_node.children) >= 3 and len(base_node.children) % 2 == 1
                    base_node.children.append(Node(oi.op.name))
                    base_node.children.append(add_node)
                    new_node = base_node
                flat_flag = True
            else:
                new_node = oi.op.to_node(*[x.node for x in atom_stack[-oi.op.arity :]])
                flat_flag = False
            new_node.name = oi.level_info.name
            atom_stack = atom_stack[: -oi.op.arity]
            atom_stack.append(AtomItem(new_node, flat_flag, token_begin, token_end))

    def hallucinate_concat():
        nonlocal mode, index
        level_info = saxe.get_concat_info()
        assert level_info is not None
        stack_pop(level_info)
        oper_stack.append(OperItem(seed_axe.Infix(None), level_info, []))
        mode = ATOM_MODE

    def handle_bracketed(left_bracket, right_bracket):
        nonlocal index
        assert index < len(tokens)
        assert tokens[index].name == left_bracket
        retval = expr_impl(saxe, tokens, index + 1)
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
                atom_stack.append(AtomItem(Node(token.name), True, index, index + 1))
                mode = INFIX_MODE
                index += 1
                continue

            if mode == INFIX_MODE and saxe.has_concat():
                hallucinate_concat()
                continue

        elif tokens[index].type == misc.TokenType.OPER:
            lr = saxe.lookup(token.name)
            if lr.is_right_bracket:
                break

            if mode == INFIX_MODE and saxe.has_concat():
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

                if type(op) == seed_axe.Prefix:
                    oper_stack.append(OperItem(op, level_info, [index], min_token_index=index))
                    index += 1
                    continue

                if type(op) == seed_axe.PrefixBracketed:
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

                if type(op) == seed_axe.Postfix:
                    oper_stack.append(OperItem(op, level_info, [index], max_token_index=index + 1))
                    index += 1
                    continue

                if type(op) == seed_axe.PostfixBracketed:
                    atom = handle_bracketed(op.left_bracket, op.right_bracket)
                    atom_stack.append(atom)
                    oper_stack.append(OperItem(op, level_info, [], max_token_index=atom.token_end))
                    continue

                if type(op) == seed_axe.Infix:
                    oper_stack.append(OperItem(op, level_info, [index]))
                    mode = ATOM_MODE
                    index += 1
                    continue

                if type(op) == seed_axe.Ternary:
                    atom_mid = handle_bracketed(op.first_name, op.second_name)
                    atom_stack.append(atom_mid)
                    oper_stack.append(OperItem(op, level_info, []))
                    mode = ATOM_MODE
                    continue

        raise Exception(f'Unknown {tokens[index]=}')

    stack_pop(seed_axe.LevelInfo('END', -1, seed_axe.Assoc.NONE))
    assert len(oper_stack) == 0, f'oper_stack not empty: {pprint.pformat(oper_stack)}'
    assert len(atom_stack) == 1, f'atom_stack not unit: {pprint.pformat(atom_stack)}'
    return atom_stack[0]


def shunting_yard(saxe: seed_axe.SeedAxe, tokens: list[misc.Token]) -> Node:
    retval = expr_impl(saxe, tokens, 0)
    assert retval.token_begin == 0
    assert retval.token_end == len(tokens)
    return retval.node


def _run():
    testset.execute(shunting_yard)


if __name__ == '__main__':
    _run()
