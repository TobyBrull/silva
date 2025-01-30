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
                        if not (token.name == lit):
                            break
                token_idx += 1
                count -= 1
            if count == 0:
                return False
        return True


class Assoc(enum.Enum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1


BINDING_POWER_INF_LEFT = 999_999
BINDING_POWER_INF_RIGHT = 1_000_000


@dataclasses.dataclass
class Prefix:
    name: str


@dataclasses.dataclass
class Infix:
    name: str | None


@dataclasses.dataclass
class Postfix:
    name: str


@dataclasses.dataclass
class PostfixBracketed:
    left_bracket: str
    right_bracket: str


@dataclasses.dataclass
class Ternary:
    first_name: str
    second_name: str


RegularOp = Infix | Ternary | Postfix | PostfixBracketed


@dataclasses.dataclass
class Level:
    assoc: Assoc
    ops: list[RegularOp | Prefix]


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
    regular_index: int | None = None
    prefix_index: int | None = None

    def _register(self, index: int, op_type: OpType):
        if op_type == OpType.PREFIX:
            assert self.prefix_index is None
            self.prefix_index = index
        else:
            assert self.regular_index is None
            self.regular_index = index


@dataclasses.dataclass
class ParseAxeKind:
    regular_prec: int | None = None
    prefix_prec: int | None = None

    def _register(self, index: int, op: RegularOp | Prefix):
        pass


def _get_op_type(op: RegularOp | Prefix) -> OpType:
    if type(op) == Prefix:
        return OpType.PREFIX
    elif type(op) == Infix:
        return OpType.INFIX
    elif type(op) == Postfix or type(op) == PostfixBracketed:
        return OpType.POSTFIX
    elif type(op) == Ternary:
        return OpType.TERNARY
    else:
        raise Exception(f'Unknown {type(op)=}')


class ParseAxe:
    def __init__(self, transparent_brackets: tuple[str, str]):
        self.levels: list[Level] = []
        self.op_map: dict[str | None, OpMapEntry] = {}
        self.kind_map: dict[str | None, ParseAxeKind] = {}
        self.transparent_brackets: tuple[str, str] = transparent_brackets

    def _add_level(self, level: Level):
        index = len(self.levels)
        self.levels.append(level)
        for op in level.ops:
            if type(op) == Prefix:
                self._add_op(op.name, index, op)
            elif type(op) == Infix:
                self._add_op(op.name, index, op)
            elif type(op) == Postfix:
                self._add_op(op.name, index, op)
            elif type(op) == PostfixBracketed:
                self._add_op(op.left_bracket, index, op)
                self._add_op(op.right_bracket, index, op)
            elif type(op) == Ternary:
                self._add_op(op.first_name, index, op)
                self._add_op(op.second_name, index, op)
            else:
                raise Exception(f'Unknown {type(op)=}')

    def _add_op(self, op_name: str | None, index: int, op: RegularOp | Prefix):
        op_type = _get_op_type(op)
        self.op_map.setdefault(op_name, OpMapEntry())._register(index, op_type)
        self.kind_map.setdefault(op_name, ParseAxeKind())._register(index, op)

    def __getitem__(self, op_name: str) -> ParseAxeKind:
        if op_name not in self.op_map:
            return ParseAxeKind()
        else:
            return self.kind_map[op_name]

    def is_right_bracket(self, op_name: str) -> bool:
        if op_name == self.transparent_brackets[1]:
            return True
        if op_name not in self.op_map:
            return False
        if (idx := self.op_map[op_name].regular_index) is not None:
            for op in self.levels[idx].ops:
                if (type(op) == Ternary and op.second_name == op_name) or (
                    type(op) == PostfixBracketed and op.right_bracket == op_name
                ):
                    return True
        return False

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

    def level_ltr(self, *ops: RegularOp):
        self.levels.append(Level(Assoc.LEFT_TO_RIGHT, [x for x in ops if x]))

    def level_rtl(self, *ops: Infix | Ternary | Prefix):
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
