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


@dataclasses.dataclass
class LevelPrefix:
    ops: set[str]


@dataclasses.dataclass
class LevelInfix:
    assoc: Assoc
    ops: set[str | Concat]


@dataclasses.dataclass
class LevelPostfix:
    ops: set[str]


@dataclasses.dataclass
class LevelPostfixExpr:
    nonterminal: str
    production: Production


@dataclasses.dataclass
class LevelPostfixBracketed:
    left_bracket: str
    right_bracket: str


@dataclasses.dataclass
class LevelTernary:
    first_op: str
    second_op: str


type Level = LevelPrefix | LevelInfix | LevelPostfix | LevelPostfixBracketed | LevelPostfixExpr | LevelTernary


BINDING_POWER_INF_LEFT = 999_999
BINDING_POWER_INF_RIGHT = 1_000_000


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
        self.op_map: dict[str | Concat, OpMapEntry] = {}

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        return index

    def _add_op(self, op: str | Concat, index: int, op_type: OpType):
        ome = self.op_map.setdefault(op, OpMapEntry())
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
        if level.second_op == op:
            return None
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
        self.levels.append(LevelPrefix(ops=set(ops)))

    def infix(self, assoc: Assoc, ops: list[str | Concat]):
        self.levels.append(LevelInfix(assoc=assoc, ops=set(ops)))

    def postfix(self, ops: list[str]):
        self.levels.append(LevelPostfix(ops=set(ops)))

    def postfix_bracketed(self, left_bracket: str, right_bracket: str):
        self.levels.append(LevelPostfixBracketed(left_bracket, right_bracket))

    def postfix_expr(self, expr_str: str, production: Production):
        self.levels.append(LevelPostfixExpr(expr_str, production))

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
            elif type(level) == LevelPostfixExpr:
                retval._add_level(level)
            elif type(level) == LevelTernary:
                index = retval._add_level(level)
                retval._add_op(level.first_op, index, OpType.TERNARY)
                retval._add_op(level.second_op, index, OpType.TERNARY)

        return retval


@dataclasses.dataclass
class Prefix:
    op: str


@dataclasses.dataclass
class Infix:
    op: str | Concat


@dataclasses.dataclass
class Postfix:
    op: str


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
    first_op: str
    second_op: str


LtrOp = Infix | Ternary | Postfix | PostfixExpr | PostfixBracketed
RtlOp = Infix | Ternary | Prefix


class ParseAxe2:
    def __init__(self):
        self.levels: list[Level] = []
        self.op_map: dict[str | Concat, OpMapEntry] = {}

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        return index

    def _add_op(self, op: str | Concat, index: int, op_type: OpType):
        ome = self.op_map.setdefault(op, OpMapEntry())
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
        if level.second_op == op:
            return None
        return (_to_bp(idx, lo=True), level.second_op)

    def precedence_climbing_infix(self, op: str) -> tuple[int, Assoc]:
        e = self.op_map[op]
        assert e.infix_index
        level = self.levels[e.infix_index]
        assert type(level) == LevelInfix
        return (e.infix_index, level.assoc)


class ParseAxeNursery2:
    def __init__(self):
        self.levels: list[tuple[Assoc, list[LtrOp | RtlOp]]] = []

    def level_ltr(self, *ops: LtrOp):
        self.levels.append((Assoc.LEFT_TO_RIGHT, [x for x in ops if x]))

    def level_rtl(self, *ops: RtlOp):
        self.levels.append((Assoc.RIGHT_TO_LEFT, [x for x in ops if x]))

    def finish(self) -> ParseAxe2:
        retval = ParseAxe2()
        for level in reversed(self.levels):
            pass
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
