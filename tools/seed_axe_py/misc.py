import dataclasses
import enum
from typing import NamedTuple


class TokenType(enum.Enum):
    ATOM = 0
    OPER = 1


class Token(NamedTuple):
    type: TokenType
    name: str


_atom_first = set([ch for ch in '_abcdefghijklmno0123456789'])


def tokenize(input_: str) -> list[Token]:
    retval = []
    for token_str in input_.split(' '):
        assert token_str, f'repeated spaces not allowed in {input_=}'
        is_atom = token_str[0] in _atom_first
        retval.append(Token(type=TokenType.ATOM if is_atom else TokenType.OPER, name=token_str))
    return retval
