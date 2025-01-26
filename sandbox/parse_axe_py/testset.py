import traceback
import termcolor
import misc
import pprint
import parse_axe


def _green(text: str) -> str:
    return termcolor.colored(text, 'green')


def _red(text: str) -> str:
    return termcolor.colored(text, 'red')


def _yellow(text: str) -> str:
    return termcolor.colored(text, 'yellow')


class _TestTracker:
    def __init__(self, parser, excluded: list[str]):
        self.parser = parser
        self.parser_name = parser.__name__
        self.full_test_names_excluded = set(excluded)

        self.curr_paxe = None
        self.curr_paxe_name = None
        self.curr_test_name = None
        self.curr_test_index = 0

        self.test_count = 0
        self.fails: list[str] = []
        self.full_test_names_attempted = set()

    def set_parse_axe(self, paxe: parse_axe.ParseAxe, paxe_name: str):
        self.curr_paxe = paxe
        self.curr_paxe_name = paxe_name

    def set_current_test_name(self, test_name: str):
        self.curr_test_index = 0
        self.curr_test_name = test_name

    def curr_full_test_name(self) -> str:
        return f'{self.curr_paxe_name}/{self.curr_test_name}'

    def __call__(self, source_code: str, expected: str | None):
        ftn = self.curr_full_test_name()
        self.full_test_names_attempted.add(ftn)
        if ftn in self.full_test_names_excluded:
            return

        self.curr_test_index += 1
        self.test_count += 1
        tokens = misc.tokenize(source_code)
        try:
            result = self.parser(self.curr_paxe, tokens)
            err_msg = ''
        except Exception as e:
            result = None
            err_msg = str(e)
            err_msg = traceback.format_exc()
        if result != expected:
            self.fails.append(f'{self.curr_test_name},{self.curr_test_index}')
            print(
                f"\n\n"
                + _red(f"ERROR")
                + f" ========= "
                + f"{self.parser_name} {self.curr_full_test_name()} [{self.curr_test_index}]"
                + f" ========== "
            )
            print('Error message:', err_msg)
            print('Source code:', source_code)
            pprint.pprint(tokens)
            print('Result:  ', result)
            print('Expected:', expected)
            print()

    def print_exit_message(self):
        if not self.fails:
            full_test_names_excluded = self.full_test_names_excluded.intersection(
                self.full_test_names_attempted
            )
            appendix = ''
            if full_test_names_excluded:
                appendix = 'Excluded: ' + _yellow(', '.join(full_test_names_excluded))
            passed_str = _green(f'all {self.test_count} tests passed!')
            print(f'{self.parser_name:20}: {passed_str:31} {appendix}')


Prefix = parse_axe.Prefix
Postfix = parse_axe.Postfix
PostfixExpr = parse_axe.PostfixExpr
PostfixBracketed = parse_axe.PostfixBracketed
Infix = parse_axe.Infix
Ternary = parse_axe.Ternary


def basic(tt: _TestTracker):
    pan = parse_axe.ParseAxeNursery()
    pan.level_rtl(Infix('.'))
    pan.level_ltr(PostfixBracketed('[', ']'))
    pan.level_ltr(Postfix('$'))
    pan.level_ltr(Postfix('!'))
    pan.level_rtl(Prefix('~'))
    pan.level_rtl(Prefix('+'), Prefix('-'))
    pan.level_ltr(Infix('*'), Infix('/'))
    pan.level_ltr(Infix('+'), Infix('-'))
    pan.level_rtl(Ternary('?', ':'))
    pan.level_rtl(Infix('='))
    paxe = pan.finish()

    # pprint.pprint(self.paxe_def.op_map)
    # pprint.pprint(self.paxe_def.levels)

    tt.set_parse_axe(paxe, "base")

    tt.set_current_test_name("infix")
    tt("1", '1')
    tt("1 + 2 * 3", '{ + 1 { * 2 3 } }')
    tt("1 + 2 * 3 + 4", '{ + { + 1 { * 2 3 } } 4 }')
    tt("a + b * c * d + e", '{ + { + a { * { * b c } d } } e }')
    tt("f . g . h", '{ . f { . g h } }')
    tt("a + b - c + d", '{ + { - { + a b } c } d }')
    tt("1 + 2 + f . g . h * 3 * 4", '{ + { + 1 2 } { * { * { . f { . g h } } 3 } 4 } }')

    tt.set_current_test_name("allfix")
    tt("2 ! + 3", '{ + { ! 2 } 3 }')
    tt('+ 1', '{ + 1 }')
    tt('- + 1', '{ - { + 1 } }')
    tt('1 + + - 1', '{ + 1 { + { - 1 } } }')
    tt("- - 1 * 2", '{ * { - { - 1 } } 2 }')
    tt("- - f . g", '{ - { - { . f g } } }')
    tt("- 9 !", '{ - { ! 9 } }')
    tt("f . g !", '{ ! { . f g } }')
    tt("f ! . g !", None)
    tt("f ! . g ! . h !", None)
    tt("f + g !", '{ + f { ! g } }')
    tt("f . g !", '{ ! { . f g } }')
    tt("f ! + g !", '{ + { ! f } { ! g } }')

    tt.set_current_test_name("parentheses")
    tt("( ( ( 0 ) ) )", '0')
    tt("( 1 + 2 ) * 3", '{ * { + 1 2 } 3 }')
    tt("1 + ( 2 * 3 )", '{ + 1 { * 2 3 } }')

    tt.set_current_test_name("subscript")
    tt("a [ 0 ]", '{ [ a 0 }')
    tt("a [ 0 ] [ 1 ]", '{ [ { [ a 0 } 1 }')
    tt("a [ 0 ] [ b [ 0 + 1 ] ]", '{ [ { [ a 0 } { [ b { + 0 1 } } }')
    tt("a [ 0 ] . b [ 0 ]", None)
    tt("a [ 0 ] + b [ 0 ]", '{ + { [ a 0 } { [ b 0 } }')

    tt.set_current_test_name("ternary")
    tt("a ? b : c", '{ ? a b c }')
    tt("a ? b : c ? d : e", '{ ? a b { ? c d e } }')
    tt("a ? b ? c : d : e", '{ ? a { ? b c d } e }')
    tt("a = b ? c : d = e", '{ = a { = { ? b c d } e } }')
    tt("a + b ? c : d + e", '{ ? { + a b } c { + d e } }')
    tt("a = b ? c = d : e = f", None)
    tt("a + b ? c + d : e + f", '{ ? { + a b } { + c d } { + e f } }')


def pq_notation(tt: _TestTracker):
    pan = parse_axe.ParseAxeNursery()
    pan.level_ltr(Postfix('q4'))
    pan.level_ltr(Postfix('q3'))
    pan.level_rtl(Prefix('p4'))
    pan.level_rtl(Prefix('p3'))
    pan.level_rtl(Infix('x2'))
    pan.level_ltr(Infix('x1'))
    pan.level_ltr(Postfix('q2'))
    pan.level_ltr(Postfix('q1'))
    pan.level_rtl(Prefix('p2'))
    pan.level_rtl(Prefix('p1'))
    paxe = pan.finish()

    tt.set_parse_axe(paxe, "pq")

    tt.set_current_test_name("allfix")
    tt('p2 p1 a', None)
    tt('p1 p2 a', '{ p1 { p2 a } }')
    tt('a q1 q2', None)
    tt('a q2 q1', '{ q1 { q2 a } }')
    tt('p3 aaa x1 bbb q3', '{ x1 { p3 aaa } { q3 bbb } }')
    tt('aaa q3 x1 bbb q2', '{ q2 { x1 { q3 aaa } bbb } }')
    tt('aaa q2 x1 bbb q3', None)


def expr_based(result_tracker: _TestTracker):
    pan = parse_axe.ParseAxeNursery()
    pan.level_ltr(PostfixExpr('Subscript', parse_axe.Production("'[' Atom Oper ']'")))
    paxe = pan.finish()


def execute(parser, excluded: list[str] = []):
    tt = _TestTracker(parser, excluded)
    basic(tt)
    pq_notation(tt)
    expr_based(tt)
    tt.print_exit_message()
