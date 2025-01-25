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

    def run_test(self, source_code: str, expected: str | None):
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
        self.paxe_def.add_level_prefix(['~'])
        self.paxe_def.add_level_postfix(['!'])
        self.paxe_def.add_level_postfix(['$'])
        self.paxe_def.add_level_postfix_expr('Subscript')
        self.paxe_def.add_level_infix(RTL, ['.'])

        self.paxe_hilo = parse_axe.ParseAxe()
        self.paxe_hilo.add_level_prefix(['p1'])
        self.paxe_hilo.add_level_prefix(['p2'])
        self.paxe_hilo.add_level_postfix(['q1'])
        self.paxe_hilo.add_level_postfix(['q2'])
        self.paxe_hilo.add_level_infix(LTR, ['x1'])
        self.paxe_hilo.add_level_infix(RTL, ['x2'])
        self.paxe_hilo.add_level_prefix(['p3'])
        self.paxe_hilo.add_level_prefix(['p4'])
        self.paxe_hilo.add_level_postfix(['q3'])
        self.paxe_hilo.add_level_postfix(['q4'])

        # pprint.pprint(self.paxe_def.op_map)
        # pprint.pprint(self.paxe_def.prec_levels)

    def infix_only(self):
        with TestRunner(self.parser, self.paxe_def, "infix_only") as tr:
            tr.run_test("1", '1')
            tr.run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
            tr.run_test("1 + 2 * 3 + 4", '(+ (+ 1 (* 2 3)) 4)')
            tr.run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
            tr.run_test("f . g . h", '(. f (. g h))')
            tr.run_test("a + b - c + d", '(+ (- (+ a b) c) d)')
            tr.run_test("1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')

    def allfix(self):
        with TestRunner(self.parser, self.paxe_def, "allfix") as tr:
            tr.run_test("2 ! + 3", '(+ (! 2) 3)')
            tr.run_test('+ 1', '(+ 1)')
            tr.run_test('- + 1', '(- (+ 1))')
            tr.run_test('1 + + - 1', '(+ 1 (+ (- 1)))')
            tr.run_test("- - 1 * 2", '(* (- (- 1)) 2)')
            tr.run_test("- - f . g", '(- (- (. f g)))')
            tr.run_test("- 9 !", '(- (! 9))')
            tr.run_test("f . g !", '(! (. f g))')
            tr.run_test("f ! . g !", None)
            tr.run_test("f ! . g ! . h !", None)
            tr.run_test("f + g !", '(+ f (! g))')
            tr.run_test("f . g !", '(! (. f g))')
            tr.run_test("f ! + g !", '(+ (! f) (! g))')

        with TestRunner(self.parser, self.paxe_hilo, "allfix2") as tr:
            tr.run_test('p2 p1 a', None)
            tr.run_test('p1 p2 a', '(p1 (p2 a))')
            tr.run_test('a q1 q2', None)
            tr.run_test('a q2 q1', '(q1 (q2 a))')
            tr.run_test('p3 aaa x1 bbb q3', '(x1 (p3 aaa) (q3 bbb))')
            tr.run_test('aaa q3 x1 bbb q2', '(q2 (x1 (q3 aaa) bbb))')
            tr.run_test('aaa q2 x1 bbb q3', None)

    def parentheses(self):
        with TestRunner(self.parser, self.paxe_def, "parentheses") as tr:
            tr.run_test("( ( ( 0 ) ) )", '0')
            tr.run_test("( 1 + 2 ) * 3", '(* (+ 1 2) 3)')
            tr.run_test("1 + ( 2 * 3 )", '(+ 1 (* 2 3))')

    def subscript(self):
        with TestRunner(self.parser, self.paxe_def, "subscript") as tr:
            tr.run_test("x [ 0 ] [ 1 ]", '(Subscript (Subscript x 0) 1)')

    def ternary(self):
        with TestRunner(self.parser, self.paxe_def, "ternary") as tr:
            tr.run_test("a ? b : c ? d : e", '(? a b (? c d e))')
            tr.run_test("a ? b ? c : d : e", '(? a (? b c d) e)')
            tr.run_test("a = b ? c : d = e", '(= a (= (? b c d) e))')
            tr.run_test("a = b ? c = d : e = f", '(= a (= (? b (= c d) e) f))')
            tr.run_test("a + b ? c : d + e", '(? (+ a b) c (+ d e))')
            tr.run_test("a + b ? c + d : e + f", '(? (+ a b) (+ c d) (+ e f))')
