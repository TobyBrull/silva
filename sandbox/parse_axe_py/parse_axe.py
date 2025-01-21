import dataclasses
from enum import Enum


class PrecLevelType(Enum):
    PREFIX = 0
    INFIX_LTR = 1
    INFIX_RTL = 2
    POSTFIX = 3


class Assoc(Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


@dataclasses.dataclass
class PrecLevel:
    base_level: int
    type: PrecLevelType
    ops: list[str]


BINDING_POWER_INF_LEFT = 999_999
BINDING_POWER_INF_RIGHT = 1_000_000


@dataclasses.dataclass
class OpMapEntry:
    prefix_base_level: int | None = None
    postfix_base_level: int | None = None
    infix_base_level: int | None = None
    infix_assoc: Assoc | None = None

    def register(self, plt: PrecLevelType, lvl: int):
        # An operator may only be prefix and infix at the same time.
        match plt:
            case PrecLevelType.PREFIX:
                assert self.postfix_base_level is None and self.prefix_base_level is None
            case PrecLevelType.INFIX_LTR | PrecLevelType.INFIX_RTL:
                assert self.infix_base_level is None and self.postfix_base_level is None
            case PrecLevelType.POSTFIX:
                assert (
                    self.prefix_base_level is None
                    and self.infix_base_level is None
                    and self.postfix_base_level is None
                )
        match plt:
            case PrecLevelType.PREFIX:
                self.prefix_base_level = lvl
            case PrecLevelType.INFIX_LTR | PrecLevelType.INFIX_RTL:
                self.infix_base_level = lvl
                self.infix_assoc = (
                    Assoc.LEFT_TO_RIGHT if plt == PrecLevelType.INFIX_LTR else Assoc.RIGHT_TO_LEFT
                )
            case PrecLevelType.POSTFIX:
                self.postfix_base_level = lvl

    def binding_power(self, prefer_prefix: bool) -> tuple[int, int]:
        if self.postfix_base_level:
            return (self.postfix_base_level, BINDING_POWER_INF_RIGHT)
        elif self.prefix_base_level and prefer_prefix:
            return (BINDING_POWER_INF_LEFT, self.prefix_base_level)
        else:
            assert self.infix_base_level
            if self.infix_assoc == Assoc.LEFT_TO_RIGHT:
                return (self.infix_base_level, self.infix_base_level + 1)
            else:
                return (self.infix_base_level + 1, self.infix_base_level)


class ParseAxe:
    def __init__(self):
        self.prec_levels: list[PrecLevel] = []
        self.op_map: dict[str, OpMapEntry] = {}

    def add_prec_level(self, prec_level_type: PrecLevelType, ops: list[str]):
        base_level = 10 * (1 + len(self.prec_levels))
        self.prec_levels.append(PrecLevel(base_level=base_level, type=prec_level_type, ops=ops))
        for op in ops:
            ome = self.op_map.setdefault(op, OpMapEntry())
            ome.register(prec_level_type, base_level)

    def infix_prec(self, op: str) -> tuple[int, Assoc]:
        e = self.op_map[op]
        assert e.infix_base_level and e.infix_assoc
        return (e.infix_base_level, e.infix_assoc)

    def binding_power(self, op: str, prefer_prefix: bool) -> tuple[int, int]:
        e = self.op_map[op]
        return e.binding_power(prefer_prefix)


def default_parse_axe():
    retval = ParseAxe()
    retval.add_prec_level(PrecLevelType.INFIX_RTL, ['='])
    retval.add_prec_level(PrecLevelType.INFIX_LTR, ['+', '-'])
    retval.add_prec_level(PrecLevelType.INFIX_LTR, ['*', '/'])
    retval.add_prec_level(PrecLevelType.PREFIX, ['+', '-'])
    retval.add_prec_level(PrecLevelType.POSTFIX, ['!'])
    retval.add_prec_level(PrecLevelType.INFIX_RTL, ['.'])
    return retval
