import dataclasses
import enum

import misc


class ProductionSymbol(enum.Enum):
    ATOM = 1
    OPER = 2
    PLUS = 3


def _map_symbol(x: str) -> str | ProductionSymbol:
    if x == "Atom":
        return ProductionSymbol.ATOM
    elif x == "Oper":
        return ProductionSymbol.OPER
    elif x == "+":
        return ProductionSymbol.PLUS
    else:
        assert len(x) >= 3 and x[0] == "'" and x[-1] == "'"
        return x[1:-1]


class Production:
    def __init__(self, prod_str: str):
        parts = prod_str.split(' ')
        self.symbols = [_map_symbol(x) for x in parts]

    def matches(self, tokens: list[misc.Token]) -> bool:
        symbol_idx = 0
        token_idx = 0
        while symbol_idx < len(self.symbols):
            if token_idx >= len(tokens):
                return False
            symbol = self.symbols[symbol_idx]
            assert symbol != ProductionSymbol.PLUS
            if (
                symbol_idx + 1 < len(self.symbols)
                and self.symbols[symbol_idx + 1] == ProductionSymbol.PLUS
            ):
                has_plus = True
                symbol_idx += 2
            else:
                has_plus = False
                symbol_idx += 1
            max_count = 100 if has_plus else 1
            count = 0
            while count < max_count and token_idx < len(tokens):
                token = tokens[token_idx]
                match symbol:
                    case ProductionSymbol.ATOM:
                        if not (token.type == misc.TokenType.ATOM):
                            break
                    case ProductionSymbol.OPER:
                        if not (token.type == misc.TokenType.OPER):
                            break
                    case _ as lit:
                        if not (token.value == lit):
                            break
                token_idx += 1
                count -= 1
            if count == 0:
                return False
        return True


class Assoc(enum.Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


@dataclasses.dataclass
class Concat:
    pass


BINDING_POWER_INF_LEFT = 999_999
BINDING_POWER_INF_RIGHT = 1_000_000


@dataclasses.dataclass
class Prefix:
    name: str


@dataclasses.dataclass
class Infix:
    name: str | Concat


@dataclasses.dataclass
class Postfix:
    name: str


@dataclasses.dataclass
class PostfixExpr:
    nonterminal: str
    production: Production


@dataclasses.dataclass
class PostfixBracketed:
    left_bracket: str
    right_bracket: str


@dataclasses.dataclass
class Ternary:
    first_name: str
    second_name: str


OpLtr = Infix | Ternary | Postfix | PostfixExpr | PostfixBracketed
OpRtl = Infix | Ternary | Prefix


@dataclasses.dataclass
class Level:
    assoc: Assoc
    ops: list[OpLtr | OpRtl]


def _to_bp(index: int, lo: bool) -> int:
    return 10 * (1 + index) + (0 if lo else 1)


class OpType(enum.Enum):
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
            if level.assoc == Assoc.LEFT_TO_RIGHT:
                return (_to_bp(self.infix_index, lo=True), _to_bp(self.infix_index, lo=False))
            else:
                return (_to_bp(self.infix_index, lo=False), _to_bp(self.infix_index, lo=True))
        elif self.ternary_index is not None:
            return (-1, -1)  # TODO
        else:
            assert False


class ParseAxe:
    def __init__(self, transparent_brackets: tuple[str, str]):
        self.levels: list[Level] = []
        self.op_map: dict[str | Concat, OpMapEntry] = {}
        self.transparent_brackets: tuple[str, str] = transparent_brackets

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        for op in level.ops:
            if type(op) == Prefix:
                self._add_op(op.name, index, OpType.PREFIX)
            elif type(op) == Infix:
                self._add_op(op.name, index, OpType.INFIX)
            elif type(op) == Postfix:
                self._add_op(op.name, index, OpType.POSTFIX)
            elif type(op) == PostfixExpr:
                pass
            elif type(op) == PostfixBracketed:
                self._add_op(op.left_bracket, index, OpType.POSTFIX)
                self._add_op(op.right_bracket, index, OpType.POSTFIX)
            elif type(op) == Ternary:
                self._add_op(op.first_name, index, OpType.TERNARY)
                self._add_op(op.second_name, index, OpType.TERNARY)
            else:
                raise Exception(f'Unknown {type(op)=}')

    def _add_op(self, op: str | Concat, index: int, op_type: OpType):
        self.op_map.setdefault(op, OpMapEntry())._register(index, op_type)

    def shuting_yard_prec(self, op_name: str, prefer_prefix: bool) -> tuple[int, int]:
        e = self.op_map[op_name]
        return e._shuting_yard_prec(prefer_prefix, self.levels)

    def pratt_prefix(self, op_name: str) -> int | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].prefix_index
        if idx is None:
            return None
        return _to_bp(idx, lo=True)

    def pratt_postfix(self, op_name: str) -> tuple[int, str | None] | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].postfix_index
        if idx is None:
            return None
        for op in self.levels[idx].ops:
            if type(op) == Postfix and op.name == op_name:
                return (_to_bp(idx, lo=True), None)
            elif type(op) == PostfixBracketed and op.left_bracket == op_name:
                return (_to_bp(idx, lo=True), op.right_bracket)
        return None

    def pratt_infix(self, op_name: str) -> tuple[int, int] | None:
        if op_name not in self.op_map:
            return None
        ome = self.op_map[op_name]
        if ome.infix_index is None:
            return None
        level = self.levels[ome.infix_index]
        ltr = level.assoc == Assoc.LEFT_TO_RIGHT
        return (_to_bp(ome.infix_index, lo=ltr), _to_bp(ome.infix_index, lo=not ltr))

    def pratt_ternary(self, op_name: str) -> tuple[int, str] | None:
        if op_name not in self.op_map:
            return None
        idx = self.op_map[op_name].ternary_index
        if idx is None:
            return None
        for op in self.levels[idx].ops:
            if type(op) == Ternary and op.first_name == op_name:
                return (_to_bp(idx, lo=True), op.second_name)
        return None

    def precedence_climbing_infix(self, op: str) -> tuple[int, Assoc]:
        e = self.op_map[op]
        assert e.infix_index
        level = self.levels[e.infix_index]
        return (e.infix_index, level.assoc)


class ParseAxeNursery:
    def __init__(self, transparent_brackets: tuple[str, str] = ("(", ")")):
        self.levels: list[Level] = []
        self.transparent_brackets = transparent_brackets

    def level_ltr(self, *ops: OpLtr):
        self.levels.append(Level(Assoc.LEFT_TO_RIGHT, [x for x in ops if x]))

    def level_rtl(self, *ops: OpRtl):
        self.levels.append(Level(Assoc.RIGHT_TO_LEFT, [x for x in ops if x]))

    def finish(self) -> ParseAxe:
        retval = ParseAxe(self.transparent_brackets)
        for level in reversed(self.levels):
            retval._add_level(level)
        return retval


def _run():
    p1 = Production("'[' Atom + ']'")
    assert p1.matches(misc.tokenize("[ ]")) == False
    assert p1.matches(misc.tokenize("[ a ]"))
    assert p1.matches(misc.tokenize("[ a b ]"))
    assert p1.matches(misc.tokenize("[ a b c ]"))
    assert p1.matches(misc.tokenize("[ a + c ]")) == False

    p2 = Production("'[' Atom Oper Atom ']'")
    assert p2.matches(misc.tokenize("[ ]")) == False
    assert p2.matches(misc.tokenize("[ a ]")) == False
    assert p2.matches(misc.tokenize("[ a + ]")) == False
    assert p2.matches(misc.tokenize("[ a + b ]"))


if __name__ == '__main__':
    _run()
