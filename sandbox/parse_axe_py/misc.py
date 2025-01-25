import re
import dataclasses
from enum import Enum
from typing import NamedTuple


def cons_str(*args):
    ll = [x for x in args if x]
    if len(ll) == 1:
        return ll[0]
    else:
        return "(" + (" ".join(str(t) for t in ll)) + ")"


class TokenType(Enum):
    ATOM = 0
    OP = 1


class Token(NamedTuple):
    type: TokenType
    value: str


_atom_first = set([ch for ch in '_abcdefghijklmno0123456789'])


@dataclasses.dataclass
class Tokenization:
    tokens: list[Token]
    token_idx: int = 0

    def is_done(self):
        return self.token_idx >= len(self.tokens)

    def curr(self):
        return self.tokens[self.token_idx]


def lexer(input_: str) -> Tokenization:
    tokens = []
    for token_str in input_.split(' '):
        assert token_str, f'invalid {input_=}'
        is_atom = token_str[0] in _atom_first
        tokens.append(Token(type=TokenType.ATOM if is_atom else TokenType.OP, value=token_str))
    retval = Tokenization(tokens)
    return retval
