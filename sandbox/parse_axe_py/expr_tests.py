import misc
import parse_axe

def default_parse_axe():
    retval = parse_axe.ParseAxe()
    retval.add_prec_level(parse_axe.PrecLevelType.INFIX_RTL, ['='])
    retval.add_prec_level(parse_axe.PrecLevelType.INFIX_LTR, ['+', '-'])
    retval.add_prec_level(parse_axe.PrecLevelType.INFIX_LTR, ['*', '/'])
    retval.add_prec_level(parse_axe.PrecLevelType.PREFIX, ['+', '-'])
    retval.add_prec_level(parse_axe.PrecLevelType.POSTFIX, ['!'])
    retval.add_prec_level(parse_axe.PrecLevelType.INFIX_RTL, ['.'])
    return retval

class TestRunner:
    def __init__(self, parser):
        self.parser = parser
        self.test_count = 0
        self.fail_count = 0
        self.paxe = default_parse_axe()

    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        print(f"{self.fail_count} of {self.test_count} tests failed")
        pass

    def run_test(self, source_code: str, expected: str):
        self.test_count += 1
        tokenization = misc.lexer(source_code)
        result = self.parser(self.paxe, tokenization)
        if result != expected:
            self.fail_count += 1
            print(f"ERROR {source_code=} {result=} {expected=}")


def all(parser):
    with TestRunner(parser) as tr:
        tr.run_test("2! + 3", '(+ (! 2) 3)')
        tr.run_test("1", '1')
        tr.run_test('+1', '(+ 1)')
        tr.run_test('1 + +-1', '(+ 1 (+ (- 1)))')
        tr.run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
        tr.run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
        tr.run_test("f . g . h", '(. f (. g h))')
        tr.run_test(" 1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')
        tr.run_test("--1 * 2", '(* (- (- 1)) 2)')
        tr.run_test("--f . g", '(- (- (. f g)))')
        tr.run_test("-9!", '(- (! 9))')
        tr.run_test("f . g !", '(! (. f g))')
        tr.run_test("(((0)))", '0')
        tr.run_test("(1 + 2) * 3", '(* (+ 1 2) 3)')
        tr.run_test("1 + (2 * 3)", '(+ 1 (* 2 3))')

def all2(parser):
    with TestRunner(parser) as tr:
        tr.run_test("1", '1')
        tr.run_test('+1', '(+ 1)')
        tr.run_test('1 + +-1', '(+ 1 (+ (- 1)))')
        tr.run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
        tr.run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
        tr.run_test("f . g . h", '(. f (. g h))')
        tr.run_test("1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')
        tr.run_test("--1 * 2", '(* (- (- 1)) 2)')
        tr.run_test("--f . g", '(- (- (. f g)))')
        tr.run_test("-9!", '(- (! 9))')
        tr.run_test("f . g !", '(! (. f g))')
        tr.run_test("(((0)))", '0')
        tr.run_test("(1 + 2) * 3", '(* (+ 1 2) 3)')
        tr.run_test("x[0][1]", '([ ([ x 0) 1)')
        tr.run_test("a ? b : c ? d:  e", '(? a b (? c d e))')
        tr.run_test("a = 0 ? b : c = d", '(= a (= (? 0 b c) d))')

def all3(parser):
    with TestRunner(parser) as tr:
        tr.run_test("1", '1')
        tr.run_test("1 + 2 + 3", '(+ (+ 1 2) 3)')
        tr.run_test("f . g . h", '(. f (. g h))')
        tr.run_test("1 + 2 * 3 + 4", '(+ (+ 1 (* 2 3)) 4)')
        tr.run_test("1 + 2 * 3", '(+ 1 (* 2 3))')
        tr.run_test("a + b * c * d + e", '(+ (+ a (* (* b c) d)) e)')
        tr.run_test("1 + 2 + f . g . h * 3 * 4", '(+ (+ 1 2) (* (* (. f (. g h)) 3) 4))')

        # tr.run_test('-+1', '(- (+ 1))')
        # tr.run_test('1 + +-1', '(+ 1 (+ (- 1)))')
        # tr.run_test("--1 * 2", '(* (- (- 1)) 2)')
        # tr.run_test("--f . g", '(- (- (. f g)))')
        # tr.run_test("-9!", '(- (! 9))')
        # tr.run_test("f . g !", '(! (. f g))')
        # tr.run_test("(((0)))", '0')
        # tr.run_test("x[0][1]", '([ ([ x 0) 1)')
        # tr.run_test("a ? b : c ? d:  e", '(? a b (? c d e))')
        # tr.run_test("a = 0 ? b : c = d", '(= a (= (? 0 b c) d))')
