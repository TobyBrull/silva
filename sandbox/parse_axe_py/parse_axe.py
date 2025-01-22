import dataclasses
from enum import Enum


class Assoc(Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


@dataclasses.dataclass
class LevelPrefix:
    ops: list[str]


@dataclasses.dataclass
class LevelInfix:
    assoc: Assoc
    ops: list[str]


@dataclasses.dataclass
class LevelPostfix:
    ops: list[str]


@dataclasses.dataclass
class LevelPostfixExpr:
    nonterminal: str


@dataclasses.dataclass
class LevelTernary:
    ops: list[tuple[str, str]]


type Level = LevelPrefix | LevelInfix | LevelPostfix | LevelPostfixExpr | LevelTernary


@dataclasses.dataclass
class PrecLevel:
    index: int
    level: Level


BINDING_POWER_INF_LEFT = 999_999
BINDING_POWER_INF_RIGHT = 1_000_000


def _to_bp(index: int, lo: bool) -> int:
    return 10 * (1 + index) + (0 if lo else 1)


class OpType(Enum):
    NONE = 0
    PREFIX = 1
    POSTFIX = 2
    INFIX = 3
    TERNARY = 4


@dataclasses.dataclass
class OpMapEntry:
    prefix_index: int | None = None
    postfix_index: int | None = None
    infix_index: int | None = None
    ternary_index: int | None = None

    def register(self, index: int, omet: OpType):
        match omet:
            case OpType.PREFIX:
                assert self.prefix_index is None
                self.prefix_index = index
            case OpType.INFIX:
                assert self.infix_index is None
                self.infix_index = index
            case OpType.POSTFIX:
                assert self.postfix_index is None
                self.postfix_index = index
            case OpType.TERNARY:
                assert self.ternary_index is None
                self.ternary_index = index

        # An operator may only be registered to a single type of operator, except that an operator is
        # allowed to be a prefix and an infix at the same time (e.g., infix and prefix '-').
        set_bits = sum(
            (
                idx is not None
                for idx in (
                    self.prefix_index,
                    self.postfix_index,
                    self.infix_index,
                    self.ternary_index,
                )
            )
        )
        assert set_bits <= 1 or (
            set_bits == 2 and self.prefix_index is not None and self.infix_index is not None
        )

    def binding_power(self, prefer_prefix: bool, prec_levels: list[PrecLevel]) -> tuple[int, int]:
        if self.postfix_index is not None:
            return (_to_bp(self.postfix_index, lo=True), BINDING_POWER_INF_RIGHT)
        elif self.prefix_index is not None and prefer_prefix:
            return (BINDING_POWER_INF_LEFT, _to_bp(self.prefix_index, lo=False))
        elif self.infix_index is not None:
            level = prec_levels[self.infix_index].level
            assert type(level) == LevelInfix, f'Found level {type(level)=}'
            if level.assoc == Assoc.LEFT_TO_RIGHT:
                return (_to_bp(self.infix_index, lo=True), _to_bp(self.infix_index, lo=False))
            else:
                return (_to_bp(self.infix_index, lo=False), _to_bp(self.infix_index, lo=True))
        elif self.ternary_index is not None:
            return (-1, -1)  # TODO
        else:
            assert False


class ParseAxe:
    def __init__(self):
        self.prec_levels: list[PrecLevel] = []
        self.op_map: dict[str, OpMapEntry] = {}

    def _add_prec_level(self, level: Level):
        index = len(self.prec_levels)
        self.prec_levels.append(PrecLevel(index=index, level=level))
        return index

    def _add_op(self, op: str, index: int, omet: OpType):
        ome = self.op_map.setdefault(op, OpMapEntry())
        ome.register(index, omet)

    def add_level_prefix(self, ops):
        index = self._add_prec_level(LevelPrefix(ops=ops))
        for op in ops:
            self._add_op(op, index, OpType.PREFIX)

    def add_level_infix(self, assoc: Assoc, ops):
        index = self._add_prec_level(LevelInfix(assoc=assoc, ops=ops))
        for op in ops:
            self._add_op(op, index, OpType.INFIX)

    def add_level_postfix(self, ops):
        index = self._add_prec_level(LevelPostfix(ops=ops))
        for op in ops:
            self._add_op(op, index, OpType.POSTFIX)

    def add_level_postfix_expr(self, expr_str):
        self._add_prec_level(LevelPostfixExpr(expr_str))

    def add_level_ternary(self, ops):
        index = self._add_prec_level(LevelTernary(ops=ops))
        for op in ops:
            self._add_op(op, index, OpType.TERNARY)

    # To be used by parsers

    def infix_prec(self, op: str) -> tuple[int, Assoc]:
        e = self.op_map[op]
        assert e.infix_index
        prec_level = self.prec_levels[e.infix_index]
        assert type(prec_level.level) == LevelInfix
        return (e.infix_index, prec_level.level.assoc)

    def binding_power(self, op: str, prefer_prefix: bool) -> tuple[int, int]:
        e = self.op_map[op]
        return e.binding_power(prefer_prefix, self.prec_levels)
