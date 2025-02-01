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


@dataclasses.dataclass
class LookupResult:
    prefix_res: tuple[Op, int, Assoc] | None = None
    regular_res: tuple[Op, int, Assoc] | None = None

    def _register(self, op: Op, prec: int, assoc: Assoc):
        if isinstance(op, PrefixOp):
            assert self.prefix_res is None
            self.prefix_res = (op, prec, assoc)
        elif isinstance(op, InfixOp | PostfixOp):
            assert self.regular_res is None
            self.regular_res = (op, prec, assoc)
        else:
            raise Exception(f'Unknown {type(op)=}')

    def transparent_brackets(self) -> tuple[str, str] | None:
        if self.prefix_res is None:
            return None
        if type(self.prefix_res[0]) != TransparentBrackets:
            return None
        return self.prefix_res[0].left_bracket, self.prefix_res[0].right_bracket


class ParseAxe:
    def __init__(self):
        self._lookup_results: dict[str, LookupResult] = {}
        self._right_brackets: set[str] = set()
        self._concat: tuple[int, Assoc] | None = None

    def _add_op(self, op: Op, prec: int, assoc: Assoc):
        if type(op) == Prefix:
            self._add_op_name(op, op.name, prec, assoc)
        elif type(op) == PrefixBracketed:
            self._add_op_name(op, op.left_bracket, prec, assoc)
            self._add_op_name(op, op.right_bracket, prec, assoc)
            self._right_brackets.add(op.right_bracket)
        elif type(op) == TransparentBrackets:
            self._add_op_name(op, op.left_bracket, prec, assoc)
            self._add_op_name(op, op.right_bracket, prec, assoc)
            self._right_brackets.add(op.right_bracket)
        elif type(op) == Infix:
            if op.name is None:
                assert self._concat is None
                self._concat = (prec, assoc)
            else:
                self._add_op_name(op, op.name, prec, assoc)
        elif type(op) == Ternary:
            self._add_op_name(op, op.first_name, prec, assoc)
            self._add_op_name(op, op.second_name, prec, assoc)
            self._right_brackets.add(op.second_name)
        elif type(op) == Postfix:
            self._add_op_name(op, op.name, prec, assoc)
        elif type(op) == PostfixBracketed:
            self._add_op_name(op, op.left_bracket, prec, assoc)
            self._add_op_name(op, op.right_bracket, prec, assoc)
            self._right_brackets.add(op.right_bracket)
        else:
            raise Exception(f'Unknown {type(op)=}')

    def _add_op_name(self, op: Op, op_name: str, prec: int, assoc: Assoc):
        lr = self._lookup_results.setdefault(op_name, LookupResult())
        lr._register(op, prec, assoc)

    def is_right_bracket(self, op_name: str) -> bool:
        return op_name in self._right_brackets

    def lookup(self, op_name: str) -> LookupResult:
        return self._lookup_results[op_name]

    def lookup_concat(self) -> tuple[int, Assoc] | None:
        return self._concat

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
        retval = ParseAxe()
        tb = TransparentBrackets(
            left_bracket=self.transparent_brackets[0],
            right_bracket=self.transparent_brackets[1],
        )
        retval._add_op(tb, -1, Assoc.LEFT_TO_RIGHT)
        for prec_m1, level in enumerate(reversed(self.levels)):
            for op in level.ops:
                retval._add_op(op, 10 * (prec_m1 + 1), level.assoc)
        return retval
