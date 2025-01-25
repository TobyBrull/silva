import termcolor
import misc
import pprint
import parse_axe


def _green(text: str) -> str:
    return termcolor.colored(text, 'green')


def _red(text: str) -> str:
    return termcolor.colored(text, 'red')


class _TestsetRunner:
    def __init__(self, testset: "Testset", paxe: parse_axe.ParseAxe, name: str):
        self.testset = testset
        self.index = 0
        self.paxe = paxe
        self.name = name
        self.testset.run_tests.append(name)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        pass

    def _run_test(self, source_code: str, expected: str | None):
        self.index += 1
        self.testset.test_count += 1
        tokens = misc.make_tokenization(source_code)
        try:
            result = self.testset.parser(self.paxe, tokens)
            err_msg = ''
        except Exception as e:
            result = None
            err_msg = str(e)
        if result != expected:
            self.testset.fails.append(f'{self.name},{self.index}')
            print(
                f"\n\n" + _red(f"ERROR") + f" ========= {self.testset.parser_name} "
                f"========= {self.name}[{self.index}]"
            )
            print('Error message: ', err_msg)
            print(source_code)
            pprint.pprint(tokens)
            print(result)
            print(expected)


class Testset:
    def __init__(self, parser):
        self.parser_name = parser.__name__
        self.parser = parser

        self.test_count = 0
        self.fails: list[str] = []
        self.run_tests = []

        RTL = parse_axe.Assoc.RIGHT_TO_LEFT
        LTR = parse_axe.Assoc.LEFT_TO_RIGHT

        pan = parse_axe.ParseAxeNursery()
        pan.infix(RTL, ['.'])
        pan.postfix_expr('Subscript')
        pan.postfix(['$'])
        pan.postfix(['!'])
        pan.prefix(['~'])
        pan.prefix(['+', '-'])
        pan.infix(LTR, ['*', '/'])
        pan.infix(LTR, ['+', '-'])
        pan.ternary('?', ':')
        pan.infix(RTL, ['='])
        self.paxe_def = pan.finish()

        pan = parse_axe.ParseAxeNursery()
        pan.postfix(['q4'])
        pan.postfix(['q3'])
        pan.prefix(['p4'])
        pan.prefix(['p3'])
        pan.infix(RTL, ['x2'])
        pan.infix(LTR, ['x1'])
        pan.postfix(['q2'])
        pan.postfix(['q1'])
        pan.prefix(['p2'])
        pan.prefix(['p1'])
        self.paxe_hilo = pan.finish()

        # pprint.pprint(self.paxe_def.op_map)
        # pprint.pprint(self.paxe_def.prec_levels)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if not self.fails:
            appendix = ','.join(self.run_tests)
            passed_str = _green(f'all {self.test_count} tests passed!')
            print(f'{self.parser_name:20}: {passed_str:31} [{appendix}]')

    def infix_only(self):
        with _TestsetRunner(self, self.paxe_def, "infix_only") as tr:
            tr._run_test("1", '1')
            tr._run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
            tr._run_test("1 + 2 * 3 + 4", '(+ (+ 1 (* 2 3)) 4)')
            tr._run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
            tr._run_test("f . g . h", '(. f (. g h))')
            tr._run_test("a + b - c + d", '(+ (- (+ a b) c) d)')
            tr._run_test("1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')

    def allfix(self):
        with _TestsetRunner(self, self.paxe_def, "allfix") as tr:
            tr._run_test("2 ! + 3", '(+ (! 2) 3)')
            tr._run_test('+ 1', '(+ 1)')
            tr._run_test('- + 1', '(- (+ 1))')
            tr._run_test('1 + + - 1', '(+ 1 (+ (- 1)))')
            tr._run_test("- - 1 * 2", '(* (- (- 1)) 2)')
            tr._run_test("- - f . g", '(- (- (. f g)))')
            tr._run_test("- 9 !", '(- (! 9))')
            tr._run_test("f . g !", '(! (. f g))')
            tr._run_test("f ! . g !", None)
            tr._run_test("f ! . g ! . h !", None)
            tr._run_test("f + g !", '(+ f (! g))')
            tr._run_test("f . g !", '(! (. f g))')
            tr._run_test("f ! + g !", '(+ (! f) (! g))')

        with _TestsetRunner(self, self.paxe_hilo, "allfix2") as tr:
            tr._run_test('p2 p1 a', None)
            tr._run_test('p1 p2 a', '(p1 (p2 a))')
            tr._run_test('a q1 q2', None)
            tr._run_test('a q2 q1', '(q1 (q2 a))')
            tr._run_test('p3 aaa x1 bbb q3', '(x1 (p3 aaa) (q3 bbb))')
            tr._run_test('aaa q3 x1 bbb q2', '(q2 (x1 (q3 aaa) bbb))')
            tr._run_test('aaa q2 x1 bbb q3', None)

    def parentheses(self):
        with _TestsetRunner(self, self.paxe_def, "parentheses") as tr:
            tr._run_test("( ( ( 0 ) ) )", '0')
            tr._run_test("( 1 + 2 ) * 3", '(* (+ 1 2) 3)')
            tr._run_test("1 + ( 2 * 3 )", '(+ 1 (* 2 3))')

    def subscript(self):
        with _TestsetRunner(self, self.paxe_def, "subscript") as tr:
            tr._run_test("x [ 0 ] [ 1 ]", '(Subscript (Subscript x 0) 1)')

    def ternary(self):
        with _TestsetRunner(self, self.paxe_def, "ternary") as tr:
            tr._run_test("a ? b : c ? d : e", '(? a b (? c d e))')
            tr._run_test("a ? b ? c : d : e", '(? a (? b c d) e)')
            tr._run_test("a = b ? c : d = e", '(= a (= (? b c d) e))')
            tr._run_test("a = b ? c = d : e = f", '(= a (= (? b (= c d) e) f))')
            tr._run_test("a + b ? c : d + e", '(? (+ a b) c (+ d e))')
            tr._run_test("a + b ? c + d : e + f", '(? (+ a b) (+ c d) (+ e f))')
