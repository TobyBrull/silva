import dataclasses
import enum


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
class TransparentBrackets:
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


PrefixOp = Prefix | PrefixBracketed | TransparentBrackets
InfixOp = Infix | Ternary
PostfixOp = Postfix | PostfixBracketed

Op = PrefixOp | InfixOp | PostfixOp


@dataclasses.dataclass
class Level:
    assoc: Assoc
    ops: list[Op]


def _to_bp(index: int, lo: bool) -> int:
    return 10 * (1 + index) + (0 if lo else 1)


@dataclasses.dataclass
class OpMapEntry:
    prefix_index: tuple[int, int] | None = None
    regular_index: tuple[int, int] | None = None

    def _register(self, index: tuple[int, int], op: Op):
        if isinstance(op, PrefixOp):
            assert self.prefix_index is None
            self.prefix_index = index
        elif isinstance(op, InfixOp | PostfixOp):
            assert self.regular_index is None
            self.regular_index = index
        else:
            raise Exception(f'Unknown {type(op)=}')


@dataclasses.dataclass
class LookupResult:
    prefix_res: tuple[Op, int, Assoc] | None
    regular_res: tuple[Op, int, Assoc] | None


class ParseAxe:
    def __init__(self, transparent_brackets: tuple[str, str]):
        self.levels: list[Level] = []
        self.concat: tuple[int, Assoc] | None = None
        self.op_map: dict[str, OpMapEntry] = {}
        self.transparent_brackets: tuple[str, str] = transparent_brackets
        self.right_brackets: set[str] = set()
        self.right_brackets.add(transparent_brackets[1])

    def _add_level(self, level: Level):
        level_index = len(self.levels)
        self.levels.append(level)
        for item_index, op in enumerate(level.ops):
            index = (level_index, item_index)
            if type(op) == Prefix:
                self._add_op(op.name, index, op)
            elif type(op) == PrefixBracketed:
                self._add_op(op.left_bracket, index, op)
                self._add_op(op.right_bracket, index, op)
                self.right_brackets.add(op.right_bracket)
            elif type(op) == Infix:
                if op.name is None:
                    assert self.concat is None
                    self.concat = (level_index, level.assoc)
                else:
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

    def _add_op(self, op_name: str, index: tuple[int, int], op: Op):
        ome = self.op_map.setdefault(op_name, OpMapEntry())
        ome._register(index, op)

    def is_right_bracket(self, op_name: str) -> bool:
        return op_name in self.right_brackets

    def lookup(self, op_name: str) -> LookupResult:
        retval = LookupResult(None, None)
        if op_name in self.op_map:
            ome = self.op_map[op_name]
            if (idx := ome.prefix_index) is not None:
                retval.prefix_res = (
                    self.levels[idx[0]].ops[idx[1]],
                    10 * (idx[0] + 1),
                    self.levels[idx[0]].assoc,
                )
            if (idx := ome.regular_index) is not None:
                retval.regular_res = (
                    self.levels[idx[0]].ops[idx[1]],
                    10 * (idx[0] + 1),
                    self.levels[idx[0]].assoc,
                )
        return retval

    def lookup_concat(self) -> tuple[int, Assoc] | None:
        if self.concat is None:
            return None
        else:
            return (10 * (self.concat[0] + 1), self.concat[1])

    def left_and_right_prec(self, prec: int, assoc: Assoc) -> tuple[int, int]:
        (left_prec, right_prec) = (
            (prec, prec + 1) if (assoc == Assoc.LEFT_TO_RIGHT) else (prec + 1, prec)
        )
        return left_prec, right_prec


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
