#!/usr/bin/env python

import parser_shunting_yard
import parser_pratt
import parser_precedence_climbing

if __name__ == '__main__':
    parser_shunting_yard._run()
    parser_pratt._run()
    parser_precedence_climbing._run()
