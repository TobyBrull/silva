#!/usr/bin/env python

import parser_shunting_yard
import parser_prec_climbing

if __name__ == '__main__':
    parser_shunting_yard._run()
    parser_prec_climbing._run()
