import misc
import pprint
import parse_axe


class TestRunner:
    def __init__(self, parser, paxe: parse_axe.ParseAxe, name: str):
        self.parser = parser
        self.test_count = 0
        self.fail_count = 0
        self.paxe = paxe
        self.name = name

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        print(f"{self.fail_count} of {self.test_count} tests failed [{self.name}]")
        pass

    def run_test(self, source_code: str, expected: str):
        self.test_count += 1
        tokenization = misc.lexer(source_code)
        result = self.parser(self.paxe, tokenization)
        if result != expected:
            self.fail_count += 1
            print(f"ERROR {source_code=} {result=} {expected=}")


class Testset:
    def __init__(self, parser):
        print(f"\nTesting {parser.__name__}")
        self.parser = parser

        RTL = parse_axe.Assoc.RIGHT_TO_LEFT
        LTR = parse_axe.Assoc.LEFT_TO_RIGHT

        self.paxe_def = parse_axe.ParseAxe()
        self.paxe_def.add_level_infix(RTL, ['='])
        self.paxe_def.add_level_ternary('?', ':')
        self.paxe_def.add_level_infix(LTR, ['+', '-'])
        self.paxe_def.add_level_infix(LTR, ['*', '/'])
        self.paxe_def.add_level_prefix(['+', '-'])
        self.paxe_def.add_level_postfix(['!'])
        self.paxe_def.add_level_postfix_expr('Subscript')
        self.paxe_def.add_level_infix(RTL, ['.'])

        # pprint.pprint(self.paxe_def.op_map)
        # pprint.pprint(self.paxe_def.prec_levels)

    def infix_only(self):
        with TestRunner(self.parser, self.paxe_def, "infix_only") as tr:
            tr.run_test("1", '1')
            tr.run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
            tr.run_test("1 + 2 * 3 + 4", '(+ (+ 1 (* 2 3)) 4)')
            tr.run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
            tr.run_test("f . g . h", '(. f (. g h))')
            tr.run_test("1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')

    def allfix(self):
        with TestRunner(self.parser, self.paxe_def, "allfix") as tr:
            tr.run_test("2! + 3", '(+ (! 2) 3)')
            tr.run_test('+1', '(+ 1)')
            tr.run_test('-+1', '(- (+ 1))')
            tr.run_test('1 + +-1', '(+ 1 (+ (- 1)))')
            tr.run_test("--1 * 2", '(* (- (- 1)) 2)')
            tr.run_test("--f . g", '(- (- (. f g)))')
            tr.run_test("-9!", '(- (! 9))')
            tr.run_test("f . g !", '(! (. f g))')

    def parentheses(self):
        with TestRunner(self.parser, self.paxe_def, "parentheses") as tr:
            tr.run_test("(((0)))", '0')
            tr.run_test("(1 + 2) * 3", '(* (+ 1 2) 3)')
            tr.run_test("1 + (2 * 3)", '(+ 1 (* 2 3))')

    def subscript(self):
        with TestRunner(self.parser, self.paxe_def, "subscript") as tr:
            tr.run_test("x[0][1]", '([ ([ x 0) 1)')

    def ternary(self):
        with TestRunner(self.parser, self.paxe_def, "ternary") as tr:
            tr.run_test("a ? b : c ? d : e", '(? a b (? c d e))')
            tr.run_test("a ? b ? c : d : e", '(? a (? b c d) e)')
            tr.run_test("a = b ? c : d = e", '(= a (= (? b c d) e))')
            tr.run_test("a = b ? c = d : e = f", '(= a (= (? b (= c d) e) f))')
            tr.run_test("a + b ? c : d + e", '(? (+ a b) c (+ d e))')
            tr.run_test("a + b ? c + d : e + f", '(? (+ a b) (+ c d) (+ e f))')
