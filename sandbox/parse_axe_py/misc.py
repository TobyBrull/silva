import dataclasses
import enum
from typing import NamedTuple


def cons_str(*args):
    ll = [x for x in args if x]
    if len(ll) == 1:
        return ll[0]
    else:
        return "{" + (" ".join(str(t) for t in ll)) + "}"


class TokenType(enum.Enum):
    ATOM = 0
    OPER = 1


class Token(NamedTuple):
    type: TokenType
    value: str


_atom_first = set([ch for ch in '_abcdefghijklmno0123456789'])


def make_tokenization(input_: str) -> list[Token]:
    retval = []
    for token_str in input_.split(' '):
        assert token_str, f'repeated spaces not allowed in {input_=}'
        is_atom = token_str[0] in _atom_first
        retval.append(Token(type=TokenType.ATOM if is_atom else TokenType.OPER, value=token_str))
    return retval


@dataclasses.dataclass
class Tokenization:
    tokens: list[Token]
    token_idx: int = 0

    def is_done(self):
        return self.token_idx >= len(self.tokens)

    def curr(self):
        return self.tokens[self.token_idx]
