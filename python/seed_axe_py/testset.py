import sys
import traceback
import pytest
import termcolor
import misc
import pprint
import seed_axe


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

        self.curr_saxe = None
        self.curr_saxe_name = None
        self.curr_test_name = None
        self.curr_test_index = 0

        self.test_count = 0
        self.failed = False
        self.full_test_names_attempted = set()

    def set_seed_axe(self, saxe: seed_axe.SeedAxe, saxe_name: str):
        self.curr_saxe = saxe
        self.curr_saxe_name = saxe_name

    def set_current_test_name(self, test_name: str):
        self.curr_test_index = 0
        self.curr_test_name = test_name

    def curr_full_test_name(self) -> str:
        return f'{self.curr_saxe_name}/{self.curr_test_name}'

    def __call__(self, source_code: str, expected: str | None):
        ftn = self.curr_full_test_name()
        self.full_test_names_attempted.add(ftn)
        if ftn in self.full_test_names_excluded:
            return

        if self.failed:
            return

        tokens = misc.tokenize(source_code)
        try:
            result_node = self.parser(self.curr_saxe, tokens)
            result_str = result_node.render()
            err_msg = ''
        except Exception as e:
            result_str = None
            err_msg = str(e)
            err_msg = traceback.format_exc()
        if result_str != expected:
            self.failed = True
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
            print('Result:  ', result_str)
            print('Expected:', expected)
            print()
        self.curr_test_index += 1
        self.test_count += 1

    def print_exit_message(self):
        if not self.failed:
            full_test_names_excluded = self.full_test_names_excluded.intersection(
                self.full_test_names_attempted
            )
            appendix = ''
            if full_test_names_excluded:
                appendix = 'Excluded: ' + _yellow(', '.join(full_test_names_excluded))
            passed_str = _green(f'all {self.test_count} tests passed!')
            print(f'{self.parser_name:20}: {passed_str:31} {appendix}')


Prefix = seed_axe.Prefix
PrefixBracketed = seed_axe.PrefixBracketed
Postfix = seed_axe.Postfix
PostfixBracketed = seed_axe.PostfixBracketed
Infix = seed_axe.Infix
Ternary = seed_axe.Ternary


def basic(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery()
    san.level_rtl('cal', Infix('.'))
    san.level_ltr('sqb', PostfixBracketed('[', ']'))
    san.level_ltr('var', Postfix('$'))
    san.level_ltr('exc', Postfix('!'))
    san.level_rtl('til', Prefix('~'))
    san.level_rtl('prf', Prefix('+'), Prefix('-'))
    san.level_ltr('mul', Infix('*'), Infix('/'))
    san.level_flat('add', Infix('+'), Infix('-'))
    san.level_rtl('ter', Ternary('?', ':'))
    san.level_rtl('eqa', Infix('='))
    saxe = san.finish()

    # pprint.pprint(saxe.op_map)
    # pprint.pprint(saxe.levels)

    tt.set_seed_axe(saxe, "base")

    tt.set_current_test_name("infix")
    tt("1", '1')
    tt("1 + 2", 'add{ 1 + 2 }')
    tt("1 + 2 * 3", 'add{ 1 + mul{ 2 * 3 } }')
    tt("f . g . h", 'cal{ f . cal{ g . h } }')

    tt.set_current_test_name("infix-flat")
    tt("1 + 2 * 3 + 4", 'add{ 1 + mul{ 2 * 3 } + 4 }')
    tt("1 + 2 + 3 - 4 + 5", 'add{ 1 + 2 + 3 - 4 + 5 }')
    tt("1 + 2 * a ! + 3 - 4 + 5", 'add{ 1 + mul{ 2 * exc{ a ! } } + 3 - 4 + 5 }')
    tt("1 + 2 * 3 + 4", 'add{ 1 + mul{ 2 * 3 } + 4 }')
    tt("a + b * c * d + e", 'add{ a + mul{ mul{ b * c } * d } + e }')
    tt("a + b - c + d", 'add{ a + b - c + d }')
    tt("1 + 2 + f . g . h * 3 * 4", 'add{ 1 + 2 + mul{ mul{ cal{ f . cal{ g . h } } * 3 } * 4 } }')

    tt.set_current_test_name("allfix")
    tt("2 ! + 3", 'add{ exc{ 2 ! } + 3 }')
    tt('+ 1', 'prf{ + 1 }')
    tt('+ ~ 1', 'prf{ + til{ ~ 1 } }')
    tt('~ + 1', None)
    tt('1 $ !', 'exc{ var{ 1 $ } ! }')
    tt('1 ! $', None)
    tt('- + 1', 'prf{ - prf{ + 1 } }')
    tt('1 + + - 1', 'add{ 1 + prf{ + prf{ - 1 } } }')
    tt("- - 1 * 2", 'mul{ prf{ - prf{ - 1 } } * 2 }')
    tt("- - f . g", 'prf{ - prf{ - cal{ f . g } } }')
    tt("- 9 !", 'prf{ - exc{ 9 ! } }')
    tt("f . g !", 'exc{ cal{ f . g } ! }')
    tt("+ f . + g", None)
    tt("+ f . + g . + h", None)
    tt("+ f + g", 'add{ prf{ + f } + g }')
    tt("+ f . g", 'prf{ + cal{ f . g } }')
    tt("+ f + + g", 'add{ prf{ + f } + prf{ + g } }')
    tt("f ! . g !", None)
    tt("f ! . g ! . h !", None)
    tt("f + g !", 'add{ f + exc{ g ! } }')
    tt("f . g !", 'exc{ cal{ f . g } ! }')
    tt("f ! + g !", 'add{ exc{ f ! } + exc{ g ! } }')

    tt.set_current_test_name("parentheses")
    tt("( ( ( 0 ) ) )", '0')
    tt("( 1 + 2 ) * 3", 'mul{ add{ 1 + 2 } * 3 }')
    tt("1 + ( 2 * 3 )", 'add{ 1 + mul{ 2 * 3 } }')

    tt.set_current_test_name("subscript")
    tt("a [ 0 ]", 'sqb{ a [ 0 ] }')
    tt("a [ 0 ] [ 1 ]", 'sqb{ sqb{ a [ 0 ] } [ 1 ] }')
    tt("a [ 0 ] [ b [ 0 + 1 ] ]", 'sqb{ sqb{ a [ 0 ] } [ sqb{ b [ add{ 0 + 1 } ] } ] }')
    tt("a [ 0 ] . b [ 0 ]", None)
    tt("a [ 0 ] + b [ 0 ]", 'add{ sqb{ a [ 0 ] } + sqb{ b [ 0 ] } }')

    tt.set_current_test_name("ternary")
    tt("a ? b : c", 'ter{ a ? b : c }')
    tt("a ? b : c ? d : e", 'ter{ a ? b : ter{ c ? d : e } }')
    tt("a ? b ? c : d : e", 'ter{ a ? ter{ b ? c : d } : e }')
    tt("a = b ? c : d = e", 'eqa{ a = eqa{ ter{ b ? c : d } = e } }')
    tt("a + b ? c : d + e", 'ter{ add{ a + b } ? c : add{ d + e } }')
    tt("a = b ? c = d : e = f", 'eqa{ a = eqa{ ter{ b ? eqa{ c = d } : e } = f } }')
    tt("a + b ? c + d : e + f", 'ter{ add{ a + b } ? add{ c + d } : add{ e + f } }')

    san = seed_axe.SeedAxeNursery()
    san.level_flat('cal', Infix('.'))
    san.level_ltr('exc', Postfix('!'))
    saxe = san.finish()

    tt.set_seed_axe(saxe, "low-postfix")

    tt.set_current_test_name("flat")
    tt("a . b . c . d", 'cal{ a . b . c . d }')
    tt("a ! . b . c . d", None)
    tt("a . b ! . c . d", None)
    tt("a . b . c . d !", 'exc{ cal{ a . b . c . d } ! }')


def pq_notation(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery()
    san.level_ltr('l1', Postfix('q4'))
    san.level_ltr('l2', Postfix('q3'))
    san.level_rtl('l3', Prefix('p4'))
    san.level_rtl('l4', Prefix('p3'))
    san.level_rtl('l5', Infix('x2'))
    san.level_ltr('l6', Infix('x1'))
    san.level_ltr('l7', Postfix('q2'))
    san.level_ltr('l8', Postfix('q1'))
    san.level_rtl('l9', Prefix('p2'))
    san.level_rtl('l10', Prefix('p1'))
    saxe = san.finish()

    tt.set_seed_axe(saxe, "pq")

    tt.set_current_test_name("allfix")
    tt('p2 p1 a', None)
    tt('p1 p2 a', 'l10{ p1 l9{ p2 a } }')
    tt('a q1 q2', None)
    tt('a q2 q1', 'l8{ l7{ a q2 } q1 }')
    tt('p3 aaa x1 bbb q3', 'l6{ l4{ p3 aaa } x1 l2{ bbb q3 } }')
    tt('aaa q3 x1 bbb q2', 'l7{ l6{ l2{ aaa q3 } x1 bbb } q2 }')
    tt('aaa q2 x1 bbb q3', None)


def ternary(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery()
    san.level_ltr('ter', Ternary('?', ':'))
    saxe = san.finish()

    tt.set_seed_axe(saxe, "ternary")

    tt.set_current_test_name("easy")
    tt('a ? b : c', 'ter{ a ? b : c }')
    tt('a ? b : c ? d : e', 'ter{ ter{ a ? b : c } ? d : e }')
    tt('a ? b ? c : d : e', 'ter{ a ? ter{ b ? c : d } : e }')


def parentheses(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery(('(..', '..)'))
    san.level_ltr('ter', Ternary('(', ')'))
    san.level_ltr('pst', PostfixBracketed('(', ')'))
    with pytest.raises(Exception):
        san.finish()

    san = seed_axe.SeedAxeNursery()
    san.level_rtl('prf', PrefixBracketed('(', ')'))
    with pytest.raises(Exception):
        san.finish()

    san = seed_axe.SeedAxeNursery(("(..", "..)"))
    san.level_rtl('prf', PrefixBracketed('(', ')'))
    saxe = san.finish()

    tt.set_seed_axe(saxe, "parens")

    tt.set_current_test_name("easy")
    tt('( b ) a', 'prf{ ( b ) a }')
    tt('a (.. b ..)', None)
    tt('( (.. b ..) ) (.. a ..)', 'prf{ ( b ) a }')

    san = seed_axe.SeedAxeNursery(("(..", "..)"))
    san.level_rtl('prf', PrefixBracketed('(', ')'))
    san.level_ltr('cat', Infix(None))
    saxe_concat = san.finish()

    tt.set_seed_axe(saxe_concat, "parens-concat")

    tt.set_current_test_name("easy")
    tt('( b ) a', 'prf{ ( b ) a }')
    tt('a ( b ) c', 'cat{ a CONCAT prf{ ( b ) c } }')
    tt('( b ) a c', 'cat{ prf{ ( b ) a } CONCAT c }')
    tt('f a ( b ) c', 'cat{ cat{ f CONCAT a } CONCAT prf{ ( b ) c } }')
    tt('f ( b ) a c', 'cat{ cat{ f CONCAT prf{ ( b ) a } } CONCAT c }')
    tt('a b', 'cat{ a CONCAT b }')
    tt('a (.. b ..)', 'cat{ a CONCAT b }')
    tt('( (.. b ..) ) (.. a ..) (.. c ..)', 'cat{ prf{ ( b ) a } CONCAT c }')

    san = seed_axe.SeedAxeNursery(("(..", "..)"))
    san.level_ltr('cat', Infix(None))
    san.level_rtl('prf', PrefixBracketed('(', ')'))
    saxe_concat_2 = san.finish()

    tt.set_seed_axe(saxe_concat_2, "parens-concat-2")

    tt.set_current_test_name("easy")
    tt('( b ) a', 'prf{ ( b ) a }')
    tt('a ( b ) c', None)
    tt('a (.. ( b ) c ..)', 'cat{ a CONCAT prf{ ( b ) c } }')
    tt('( b ) a c', 'prf{ ( b ) cat{ a CONCAT c } }')
    tt('f a ( b ) c', None)
    tt('f ( b ) a c', None)
    tt('f a (.. ( b ) c ..)', 'cat{ cat{ f CONCAT a } CONCAT prf{ ( b ) c } }')
    tt('f (.. ( b ) a ..) c', 'cat{ cat{ f CONCAT prf{ ( b ) a } } CONCAT c }')
    tt('a b', 'cat{ a CONCAT b }')
    tt('a (.. b ..)', 'cat{ a CONCAT b }')
    tt('( (.. b ..) ) (.. a ..) (.. c ..)', 'prf{ ( b ) cat{ a CONCAT c } }')


def concat(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery()
    san.level_rtl('fnc', Infix('.'))
    san.level_ltr('exc', Postfix('!'))
    san.level_rtl('tld', Prefix('~'))
    san.level_ltr('add', Infix('+'))
    san.level_ltr('ifx', Infix(None), Infix('*'))
    san.level_ltr('qus', Postfix('?'))
    san.level_rtl('prf', Prefix('-'))
    san.level_rtl('eqa', Infix('='))
    saxe = san.finish()

    tt.set_seed_axe(saxe, "concat")

    tt.set_current_test_name("easy")
    tt('a b', "ifx{ a CONCAT b }")
    tt('a b c', "ifx{ ifx{ a CONCAT b } CONCAT c }")
    tt('a b * c d', "ifx{ ifx{ ifx{ a CONCAT b } * c } CONCAT d }")
    tt('a b . c d', "ifx{ ifx{ a CONCAT fnc{ b . c } } CONCAT d }")
    tt('a b = c d', "eqa{ ifx{ a CONCAT b } = ifx{ c CONCAT d } }")
    tt('~ a b', "ifx{ tld{ ~ a } CONCAT b }")
    tt('- a b', "prf{ - ifx{ a CONCAT b } }")
    tt('a b !', "ifx{ a CONCAT exc{ b ! } }")
    tt('a b ?', "qus{ ifx{ a CONCAT b } ? }")
    tt('a ~ b', "ifx{ a CONCAT tld{ ~ b } }")
    tt('a - b', None)
    tt('a ! b', "ifx{ exc{ a ! } CONCAT b }")
    tt('a ? b', None)

    san = seed_axe.SeedAxeNursery()
    san.level_rtl('fnc', Infix('.'))
    san.level_ltr('exc', Postfix('!'))
    san.level_rtl('tld', Prefix('~'))
    san.level_ltr('add', Infix('+'), Infix('-'))
    san.level_rtl('ifx', Infix(None), Infix('*'))
    san.level_ltr('qus', Postfix('?'))
    san.level_rtl('prf', Prefix('-'))
    san.level_rtl('eqa', Infix('='))
    saxe_rtl = san.finish()

    tt.set_seed_axe(saxe_rtl, "concat_rtl")

    tt.set_current_test_name("easy")
    tt('a b', "ifx{ a CONCAT b }")
    tt('a - b', "add{ a - b }")
    tt('a ( - b )', "ifx{ a CONCAT prf{ - b } }")
    tt('a b c', "ifx{ a CONCAT ifx{ b CONCAT c } }")


def cpp(tt: _TestTracker):
    san = seed_axe.SeedAxeNursery()
    san.level_ltr('nam', Infix('::'))
    san.level_ltr(
        'pst',
        Postfix('++'),
        Postfix('--'),
        PostfixBracketed('(', ')'),
        PostfixBracketed('[', ']'),
        Infix('.'),
        Infix('->'),
    )
    san.level_rtl(
        'prf',
        Prefix('++'),
        Prefix('--'),
        PrefixBracketed('<.', '.>'),  # C-style type cast
        Prefix('+'),
        Prefix('-'),
        Prefix('!'),
        Prefix('~'),
        Prefix('*'),
        Prefix('&'),
        Prefix('sizeof'),
        Prefix('new'),
    )
    san.level_ltr('mem', Infix('.*'), Infix('->*'))
    san.level_ltr('mul', Infix('*'), Infix('/'), Infix('%'))
    san.level_ltr('add', Infix('+'), Infix('-'))
    san.level_ltr('sft', Infix('<<'), Infix('>>'))
    san.level_ltr('spc', Infix('<=>'))
    san.level_ltr('cmp', Infix('<'), Infix('<='), Infix('>'), Infix('>='))
    san.level_ltr('eqa', Infix('=='), Infix('!='))
    san.level_ltr('ban', Infix('&'))
    san.level_ltr('xor', Infix('^'))
    san.level_ltr('bor', Infix('|'))
    san.level_ltr('lan', Infix('&&'))
    san.level_ltr('lor', Infix('||'))
    san.level_rtl('asg', Ternary('?', ':'), Prefix('throw'), Infix('='), Infix('+='), Infix('-='))
    san.level_ltr('com', Infix(','))
    saxe_cpp = san.finish()

    tt.set_seed_axe(saxe_cpp, "C++")

    tt.set_current_test_name("basic")
    tt('++ a', "prf{ ++ a }")
    tt('a --', "pst{ a -- }")
    tt('++ a --', "prf{ ++ pst{ a -- } }")
    tt('-- a ++', "prf{ -- pst{ a ++ } }")
    tt('a ( b , c )', "pst{ a ( com{ b , c } ) }")
    tt('a ( b , c , d )', "pst{ a ( com{ com{ b , c } , d } ) }")
    tt('a + ( b , c , d )', "add{ a + com{ com{ b , c } , d } }")
    tt('a ( ( b , c ) )', "pst{ a ( com{ b , c } ) }")
    tt('sizeof a', "prf{ sizeof a }")
    tt('sizeof ( a )', "prf{ sizeof a }")
    tt('a + ( b + c )', "add{ a + add{ b + c } }")
    tt('a ( b + c )', "pst{ a ( add{ b + c } ) }")
    tt('( int ) a', None)
    tt('int a', None)
    tt('a < b', 'cmp{ a < b }')
    tt('a > b', 'cmp{ a > b }')
    tt('<. int .> a', "prf{ <. int .> a }")


def execute(parser, excluded: list[str] = []):
    tt = _TestTracker(parser, excluded)
    basic(tt)
    pq_notation(tt)
    ternary(tt)
    parentheses(tt)
    concat(tt)
    cpp(tt)
    tt.print_exit_message()
