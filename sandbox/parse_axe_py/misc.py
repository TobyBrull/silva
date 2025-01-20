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


_regexes = [
    ('ATOM', r'[0-9A-Za-z]'),
    ('OP', r'[-+=?*/.![\]:()]'),
]
_scanner = re.compile("|".join(f"(?P<{name}>{value})" for (name, value) in _regexes))
_types = tuple(TokenType)


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
    for match in _scanner.finditer(input_):
        i = match.lastindex
        assert i
        text = match[i]
        tokens.append(Token(_types[i - 1], text))
    retval = Tokenization(tokens)
    return retval
