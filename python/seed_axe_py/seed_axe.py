import dataclasses
import enum

import parse_tree


class Assoc(enum.Enum):
    NONE = 0
    LEFT_TO_RIGHT = 1
    RIGHT_TO_LEFT = 2
    FLAT = 3


Node = parse_tree.Node


@dataclasses.dataclass
class Prefix:
    name: str

    arity: int = 1

    def to_node(self, arg: Node) -> Node:
        return Node(children=[Node(self.name), arg])


@dataclasses.dataclass
class PrefixBracketed:
    left_bracket: str
    right_bracket: str

    arity: int = 2

    def to_node(self, arg1: Node, arg2: Node) -> Node:
        return Node(children=[Node(self.left_bracket), arg1, Node(self.right_bracket), arg2])


@dataclasses.dataclass
class TransparentBrackets:
    left_bracket: str
    right_bracket: str

    arity: int = 0

    def to_node(self, *args) -> Node:
        raise Exception(f'Unexpected')


@dataclasses.dataclass
class Infix:
    name: str | None

    arity: int = 2

    def to_node(self, arg1: Node, arg2: Node) -> Node:
        return Node(children=[arg1, Node(self.name), arg2])


@dataclasses.dataclass
class Ternary:
    first_name: str
    second_name: str

    arity: int = 3

    def to_node(self, arg1: Node, arg2: Node, arg3: Node) -> Node:
        return Node(children=[arg1, Node(self.first_name), arg2, Node(self.second_name), arg3])


@dataclasses.dataclass
class Postfix:
    name: str

    arity: int = 1

    def to_node(self, arg: Node) -> Node:
        return Node(children=[arg, Node(self.name)])


@dataclasses.dataclass
class PostfixBracketed:
    left_bracket: str
    right_bracket: str

    arity: int = 2

    def to_node(self, arg1: Node, arg2: Node) -> Node:
        return Node(children=[arg1, Node(self.left_bracket), arg2, Node(self.right_bracket)])


PrefixOp = Prefix | PrefixBracketed | TransparentBrackets
InfixOp = Infix | Ternary
PostfixOp = Postfix | PostfixBracketed

Op = PrefixOp | InfixOp | PostfixOp


@dataclasses.dataclass
class LevelInfo:
    name: str
    prec: int
    assoc: Assoc

    def __lt__(self, other):
        if self.prec < other.prec:
            return True
        elif self.prec > other.prec:
            return False
        else:
            assert self.assoc == other.assoc
            assoc = self.assoc
            return (assoc == Assoc.RIGHT_TO_LEFT)


@dataclasses.dataclass
class LookupResult:
    prefix_res: tuple[Op, LevelInfo] | None = None
    regular_res: tuple[Op, LevelInfo] | None = None
    is_right_bracket: bool = False

    def _register(self, op: Op, level_info: LevelInfo):
        if isinstance(op, PrefixOp):
            assert self.prefix_res is None
            assert self.is_right_bracket == False
            self.prefix_res = (op, level_info)
        elif isinstance(op, InfixOp | PostfixOp):
            assert self.regular_res is None
            assert self.is_right_bracket == False
            self.regular_res = (op, level_info)
        else:
            raise Exception(f'Unknown {type(op)=}')

    def _register_right_bracket(self):
        assert self.prefix_res is None, f'{str(self.prefix_res)}'
        assert self.regular_res is None, f'{str(self.regular_res)}'
        self.is_right_bracket = True

    def transparent_brackets(self) -> tuple[str, str] | None:
        if self.prefix_res is None:
            return None
        if type(self.prefix_res[0]) != TransparentBrackets:
            return None
        return self.prefix_res[0].left_bracket, self.prefix_res[0].right_bracket


class SeedAxe:
    def __init__(self):
        self._lookup_results: dict[str, LookupResult] = {}
        self._concat: LevelInfo | None = None

    def _add_op(self, op: Op, level_info: LevelInfo):
        if type(op) == Prefix:
            self._add_op_name(op, op.name, level_info)
        elif type(op) == PrefixBracketed:
            self._add_op_name(op, op.left_bracket, level_info)
            self._add_op_right_bracket(op.right_bracket)
        elif type(op) == TransparentBrackets:
            self._add_op_name(op, op.left_bracket, level_info)
            self._add_op_right_bracket(op.right_bracket)
        elif type(op) == Infix:
            if op.name is None:
                assert self._concat is None
                self._concat = level_info
            else:
                self._add_op_name(op, op.name, level_info)
        elif type(op) == Ternary:
            self._add_op_name(op, op.first_name, level_info)
            self._add_op_right_bracket(op.second_name)
        elif type(op) == Postfix:
            self._add_op_name(op, op.name, level_info)
        elif type(op) == PostfixBracketed:
            self._add_op_name(op, op.left_bracket, level_info)
            self._add_op_right_bracket(op.right_bracket)
        else:
            raise Exception(f'Unknown {type(op)=}')

    def _add_op_name(self, op: Op, op_name: str, level_info: LevelInfo):
        lr = self._lookup_results.setdefault(op_name, LookupResult())
        lr._register(op, level_info)

    def _add_op_right_bracket(self, op_name: str):
        lr = self._lookup_results.setdefault(op_name, LookupResult())
        try:
            lr._register_right_bracket()
        except Exception as e:
            raise Exception(f'{op_name=}') from e

    def lookup(self, op_name: str) -> LookupResult:
        return self._lookup_results[op_name]

    def get_concat_info(self) -> LevelInfo | None:
        return self._concat

    def has_concat(self) -> bool:
        return self._concat is not None


@dataclasses.dataclass
class _Level:
    info: LevelInfo
    ops: list[Op]


class SeedAxeNursery:
    def __init__(self, transparent_brackets: tuple[str, str] = ("(", ")")):
        self.levels: list[_Level] = []
        self.transparent_brackets = transparent_brackets

    def level_ltr(self, level_name: str, *ops: InfixOp | PostfixOp):
        self.levels.append(
            _Level(
                LevelInfo(level_name, -1, Assoc.LEFT_TO_RIGHT),
                [x for x in ops if x],
            )
        )

    def level_rtl(self, level_name: str, *ops: PrefixOp | InfixOp):
        self.levels.append(
            _Level(
                LevelInfo(level_name, -1, Assoc.RIGHT_TO_LEFT),
                [x for x in ops if x],
            )
        )

    def level_flat(self, level_name: str, *ops: InfixOp):
        self.levels.append(
            _Level(
                LevelInfo(level_name, -1, Assoc.FLAT),
                [x for x in ops if x],
            )
        )

    def finish(self) -> SeedAxe:
        retval = SeedAxe()
        tb = TransparentBrackets(
            left_bracket=self.transparent_brackets[0],
            right_bracket=self.transparent_brackets[1],
        )
        retval._add_op(tb, LevelInfo('trn', 1_000_000_000, Assoc.NONE))
        for prec_m1, level in enumerate(reversed(self.levels)):
            level.info.prec = prec_m1 + 1
            for op in level.ops:
                retval._add_op(op, level.info)
        return retval


class ParseMode(enum.Enum):

    # In the mode where an atom is naturally expected.
    # In this mode, a prefix operator may also appear next.
    ATOM = 1

    # In the mode where an infix operator is naturally expected.
    # In this mode, a postfix operator may also appear next.
    # Also, if concatenation is allowed, an atom or prefix may appear next, in which case a CONCAT
    # operator is hallucinated.
    INFIX = 2
