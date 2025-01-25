import dataclasses
from enum import Enum


class Assoc(Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


@dataclasses.dataclass
class Concat:
    pass


@dataclasses.dataclass
class LevelPrefix:
    ops: list[str]


@dataclasses.dataclass
class LevelInfix:
    assoc: Assoc
    ops: list[str | Concat]


@dataclasses.dataclass
class LevelPostfix:
    ops: list[str]


@dataclasses.dataclass
class LevelPostfixExpr:
    nonterminal: str


@dataclasses.dataclass
class LevelTernary:
    first_op: str
    second_op: str


type Level = LevelPrefix | LevelInfix | LevelPostfix | LevelPostfixExpr | LevelTernary


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
class _OpMapEntry:
    prefix_index: int | None = None
    postfix_index: int | None = None
    infix_index: int | None = None
    ternary_index: int | None = None

    def _register(self, index: int, op_type: OpType):
        match op_type:
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

    def _shuting_yard_prec(self, prefer_prefix: bool, levels: list[Level]) -> tuple[int, int]:
        if self.postfix_index is not None:
            return (_to_bp(self.postfix_index, lo=True), BINDING_POWER_INF_RIGHT)
        elif self.prefix_index is not None and prefer_prefix:
            return (BINDING_POWER_INF_LEFT, _to_bp(self.prefix_index, lo=False))
        elif self.infix_index is not None:
            level = levels[self.infix_index]
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
        self.levels: list[Level] = []
        self.op_map: dict[str | Concat, _OpMapEntry] = {}

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        return index

    def _add_op(self, op: str | Concat, index: int, op_type: OpType):
        ome = self.op_map.setdefault(op, _OpMapEntry())
        ome._register(index, op_type)

    def shuting_yard_prec(self, op: str, prefer_prefix: bool) -> tuple[int, int]:
        e = self.op_map[op]
        return e._shuting_yard_prec(prefer_prefix, self.levels)

    def pratt_prefix(self, op: str) -> int | None:
        if op not in self.op_map:
            return None
        idx = self.op_map[op].prefix_index
        if idx is None:
            return None
        return _to_bp(idx, lo=True)

    def pratt_postfix(self, op: str) -> int | None:
        if op not in self.op_map:
            return None
        idx = self.op_map[op].postfix_index
        if idx is None:
            return None
        return _to_bp(idx, lo=True)

    def pratt_infix(self, op: str) -> tuple[int, int] | None:
        if op not in self.op_map:
            return None
        ome = self.op_map[op]
        if ome.infix_index is None:
            return None
        level = self.levels[ome.infix_index]
        assert type(level) == LevelInfix, f'{type(level)=} {op=}'
        ltr = level.assoc == Assoc.LEFT_TO_RIGHT
        return (_to_bp(ome.infix_index, lo=ltr), _to_bp(ome.infix_index, lo=not ltr))

    def pratt_ternary(self, op: str) -> tuple[int, str] | None:
        if op not in self.op_map:
            return None
        idx = self.op_map[op].ternary_index
        if idx is None:
            return None
        level = self.levels[idx]
        assert type(level) == LevelTernary
        return (_to_bp(idx, lo=True), level.second_op)

    def precedence_climbing_infix(self, op: str) -> tuple[int, Assoc]:
        e = self.op_map[op]
        assert e.infix_index
        level = self.levels[e.infix_index]
        assert type(level) == LevelInfix
        return (e.infix_index, level.assoc)


class ParseAxeNursery:
    def __init__(self):
        self.levels: list[Level] = []

    def prefix(self, ops: list[str]):
        self.levels.append(LevelPrefix(ops=ops))

    def infix(self, assoc: Assoc, ops: list[str | Concat]):
        self.levels.append(LevelInfix(assoc=assoc, ops=ops))

    def postfix(self, ops: list[str]):
        self.levels.append(LevelPostfix(ops=ops))

    def postfix_expr(self, expr_str: str):
        self.levels.append(LevelPostfixExpr(expr_str))

    def ternary(self, first_op: str, second_op: str):
        self.levels.append(LevelTernary(first_op=first_op, second_op=second_op))

    def finish(self) -> ParseAxe:
        retval = ParseAxe()
        for level in reversed(self.levels):
            if type(level) == LevelPrefix:
                index = retval._add_level(level)
                for op in level.ops:
                    retval._add_op(op, index, OpType.PREFIX)
            elif type(level) == LevelInfix:
                index = retval._add_level(level)
                for op in level.ops:
                    retval._add_op(op, index, OpType.INFIX)
            elif type(level) == LevelPostfix:
                index = retval._add_level(level)
                for op in level.ops:
                    retval._add_op(op, index, OpType.POSTFIX)
            elif type(level) == LevelPostfixExpr:
                retval._add_level(level)
            elif type(level) == LevelTernary:
                index = retval._add_level(level)
                retval._add_op(level.first_op, index, OpType.TERNARY)

        return retval
