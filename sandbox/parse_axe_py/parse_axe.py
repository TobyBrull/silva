from dataclasses import dataclass
from enum import Enum


class PrecLevelType(Enum):
    PREFIX = 1
    POSTFIX = 2
    INFIX_LTR = 3
    INFIX_RTL = 4


@dataclass
class PrecLevel:
    type: PrecLevelType
    ops: list[str]


class ParseAxe:
    def __init__(self, prec_spec):
        self.prec_spec = prec_spec
        self.infix_map = {}
        for idx, prec_level in enumerate(prec_spec):
            op_prec = len(prec_spec) - idx
            for op in prec_level.ops:
                assert op not in self.infix_map
                self.infix_map[op] = (op_prec, prec_level.type)

    def infix_prec(self, op: str) -> tuple[int, PrecLevelType]:
        retval = self.infix_map.get(op, None)
        assert retval
        return retval


def default_parse_axe():
    # High precedence first
    prec_spec = [
        PrecLevel(PrecLevelType.INFIX_RTL, ['.']),
        PrecLevel(PrecLevelType.INFIX_LTR, ['*', '/']),
        PrecLevel(PrecLevelType.INFIX_LTR, ['+', '-']),
        PrecLevel(PrecLevelType.INFIX_RTL, ['=']),
    ]
    return ParseAxe(prec_spec)
