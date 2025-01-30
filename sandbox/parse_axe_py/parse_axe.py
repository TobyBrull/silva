import dataclasses
import enum

import misc


class Assoc(enum.Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


@dataclasses.dataclass
class Prefix:
    name: str


@dataclasses.dataclass
class PrefixBracketed:
    left_bracket: str
    right_bracket: str


@dataclasses.dataclass
class Infix:
    name: str | None


@dataclasses.dataclass
class Ternary:
    first_name: str
    second_name: str


@dataclasses.dataclass
class Postfix:
    name: str


@dataclasses.dataclass
class PostfixBracketed:
    left_bracket: str
    right_bracket: str


PrefixOp = Prefix | PrefixBracketed
InfixOp = Infix | Ternary
PostfixOp = Postfix | PostfixBracketed


@dataclasses.dataclass
class Level:
    assoc: Assoc
    ops: list[PrefixOp | InfixOp | PostfixOp]


def _to_bp(index: int, lo: bool) -> int:
    return 10 * (1 + index) + (0 if lo else 1)


@dataclasses.dataclass
class OpMapEntry:
    prefix_index: int | None = None
    regular_index: int | None = None

    def _register(self, index: int, op: PrefixOp | InfixOp | PostfixOp):
        if isinstance(op, PrefixOp):
            assert self.prefix_index is None
            self.prefix_index = index
        elif isinstance(op, InfixOp | PostfixOp):
            assert self.regular_index is None
            self.regular_index = index
        else:
            raise Exception(f'Unknown {type(op)=}')


class ParseAxe:
    def __init__(self, transparent_brackets: tuple[str, str]):
        self.levels: list[Level] = []
        self.op_map: dict[str | None, OpMapEntry] = {}
        self.transparent_brackets: tuple[str, str] = transparent_brackets
        self.right_brackets: set[str] = set()
        self.right_brackets.add(transparent_brackets[1])

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        for op in level.ops:
            if type(op) == Prefix:
                self._add_op(op.name, index, op)
            elif type(op) == PrefixBracketed:
                self._add_op(op.left_bracket, index, op)
                self._add_op(op.right_bracket, index, op)
                self.right_brackets.add(op.right_bracket)
            elif type(op) == Infix:
                self._add_op(op.name, index, op)
            elif type(op) == Ternary:
                self._add_op(op.first_name, index, op)
                self._add_op(op.second_name, index, op)
                self.right_brackets.add(op.second_name)
            elif type(op) == Postfix:
                self._add_op(op.name, index, op)
            elif type(op) == PostfixBracketed:
                self._add_op(op.left_bracket, index, op)
                self._add_op(op.right_bracket, index, op)
                self.right_brackets.add(op.right_bracket)
            else:
                raise Exception(f'Unknown {type(op)=}')

    def _add_op(self, op_name: str | None, index: int, op: PrefixOp | InfixOp | PostfixOp):
        self.op_map.setdefault(op_name, OpMapEntry())._register(index, op)

    def is_right_bracket(self, op_name: str) -> bool:
        return op_name in self.right_brackets

    def prec_prefix(self, op_name: str) -> int | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].prefix_index
        if idx is None:
            return None
        return _to_bp(idx, lo=True)

    def prec_postfix(self, op_name: str) -> int | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].regular_index
        if idx is None:
            return None
        for op in self.levels[idx].ops:
            if type(op) == Postfix and op.name == op_name:
                return _to_bp(idx, lo=True)
        return None

    def prec_postfix_bracketed(self, op_name: str) -> tuple[int, str] | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].regular_index
        if idx is None:
            return None
        for op in self.levels[idx].ops:
            if type(op) == PostfixBracketed and op.left_bracket == op_name:
                return (_to_bp(idx, lo=True), op.right_bracket)
        return None

    def prec_infix(self, op_name: str | None) -> tuple[int, int] | None:
        if op_name is None:
            return self._prec_concat()
        if op_name not in self.op_map:
            return None
        ome = self.op_map[op_name]
        if ome.regular_index is None:
            return None
        level = self.levels[ome.regular_index]
        ltr = level.assoc == Assoc.LEFT_TO_RIGHT
        for op in self.levels[ome.regular_index].ops:
            if type(op) == Infix:
                return (_to_bp(ome.regular_index, lo=ltr), _to_bp(ome.regular_index, lo=not ltr))
        return None

    def _prec_concat(self) -> tuple[int, int] | None:
        if None not in self.op_map:
            return None
        idx = self.op_map[None].regular_index
        assert idx is not None
        level = self.levels[idx]
        ltr = level.assoc == Assoc.LEFT_TO_RIGHT
        return (_to_bp(idx, lo=ltr), _to_bp(idx, lo=not ltr))

    def prec_ternary(self, op_name: str) -> tuple[int, str] | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].regular_index
        if idx is None:
            return None
        for op in self.levels[idx].ops:
            if type(op) == Ternary and op.first_name == op_name:
                return (_to_bp(idx, lo=True), op.second_name)
        return None


class ParseAxeNursery:
    def __init__(self, transparent_brackets: tuple[str, str] = ("(", ")")):
        self.levels: list[Level] = []
        self.transparent_brackets = transparent_brackets

    def level_ltr(self, *ops: InfixOp | PostfixOp):
        self.levels.append(Level(Assoc.LEFT_TO_RIGHT, [x for x in ops if x]))

    def level_rtl(self, *ops: PrefixOp | InfixOp):
        self.levels.append(Level(Assoc.RIGHT_TO_LEFT, [x for x in ops if x]))

    def finish(self) -> ParseAxe:
        retval = ParseAxe(self.transparent_brackets)
        for level in reversed(self.levels):
            retval._add_level(level)
        return retval
