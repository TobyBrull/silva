import misc
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

        self.paxe_def = parse_axe.ParseAxe()
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.INFIX_RTL, ['='])
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.INFIX_LTR, ['+', '-'])
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.INFIX_LTR, ['*', '/'])
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.PREFIX, ['+', '-'])
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.POSTFIX, ['!'])
        self.paxe_def.add_prec_level(parse_axe.PrecLevelType.INFIX_RTL, ['.'])

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
            tr.run_test("a ? b : c ? d:  e", '(? a b (? c d e))')
            tr.run_test("a = 0 ? b : c = d", '(= a (= (? 0 b c) d))')
