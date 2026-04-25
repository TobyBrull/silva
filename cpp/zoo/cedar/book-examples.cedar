# From: https://github.com/nlsandler/writing-a-c-compiler-tests
#
# MIT License
#
# Copyright(c) 2023 Nora Sandler
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/ or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

int
test_func_1(void)
{
  return 100;
}

int
test_func_2(void)
{
  return 0;
}

int a = 5;

int
return_a(void)
{
  return a;
}

int
test_func_3(void)
{
  int a = 3;
  {
    extern int a;
    if (a != 5)
      return 1;
    a = 4;
  }
  return a + return_a();
}

int
test_func_4(void)
{
  int outer = 1;
  int foo   = 0;
  if (outer) {
    extern int foo;
    extern int foo;
    return foo;
  }
  return 0;
}

int foo_1 = 3;
static int foo_2;

int
test_func_5(void)
{
  return foo_2;
}

extern int foo_3;
static int foo_4 = 4;

int
foo_5(void)
{
  static int a = 3;
  a            = a * 2;
  return a;
}

int
bar(void)
{
  static int a = 4;
  a            = a + 1;
  return a;
}

int
test_func_6(void)
{
  return foo_5() + bar() + foo_5() + bar();
}

extern int zed;
int
foo_6(int a, int b, int c, int d, int e, int f, int g)
{
  return g + 1;
}

int
test_func_7(void)
{
  return foo_6(0, 0, 0, 0, 0, 0, zed);
}

int i;

int
update_static_or_global(int update_global, int new_val)
{
  static int i;
  if (update_global) {
    extern int i;
    i = new_val;
  }
  else
    i = new_val;

  return i;
}

int
test_func_8(void)
{
  if (i != 0)
    return 1;

  int result = update_static_or_global(1, 10);

  if (result != 0)
    return 1;

  if (i != 10)
    return 1;

  result = update_static_or_global(0, 9);
  if (result != 9)
    return 1;
  if (i != 10)
    return 1;

  result = update_static_or_global(1, 11);
  if (result != 9)
    return 1;
  if (i != 11)
    return 1;
  return 0;
}

int putchar(int ch);

int
print_letters(void)
{
  static int i = 65;

  putchar(i);
  {
    i = i + 1;

    static int i = 97;

    putchar(i);

    i = i + 1;
  }

  putchar(10);
  return 0;
}

void
test_func_9(void)
{
  for (int i = 0; i < 26; i = i + 1)
    print_letters();
}

int
foo_7(void)
{
  static int x;
  x = x + 1;
  return x;
}

int
test_func_10(void)
{
  int ret;
  for (int i = 0; i < 4; i = i + 1)
    ret = foo_5();
  return ret;
}

int putchar(int ch);

int
print_alphabet(void)
{
  static int count = 0;
  putchar(count + 65);
  count = count + 1;
  if (count < 26) {
    print_alphabet();
  }
  return count;
}

void
test_func_11(void)
{
  print_alphabet();
}
static int foo_8 = 3;

int
test_func_12(void)
{
  return foo_8;
}

extern int foo_9;

int
test_func_13(void)
{
  static int i = 2;
  static int j = 3;
  int cmp      = i < j;

  if (!cmp)
    return 1;
  return 0;
}

extern int foo_10;

int foo_11;

int foo_12;

int
test_func_14(void)
{
  for (int i = 0; i < 5; i = i + 1)
    foo_12 = foo_12 + 1;
  return foo_12;
}

int foo_13;
static int foo_15(void)
{
  return 3;
}

static int bar_2 = 4;

int
test_func_15(void)
{
  return foo_15() + bar_2;
}
int
test_func_16(void)
{
  return ~12;
}
int
test_func_17(void)
{
  return ~-2147483647;
}
int
test_func_18(void)
{
  return ~0;
}
int
test_func_19(void)
{
  return -5;
}
int
test_func_20(void)
{
  return -0;
}
int
test_func_21(void)
{
  return -2147483647;
}
int
test_func_22(void)
{
  return ~-3;
}
int
test_func_23(void)
{
  return -~0;
}
int
test_func_24(void)
{
  return (-2);
}
int
test_func_25(void)
{
  return ~(2);
}
int
test_func_26(void)
{
  return -(-4);
}
int
test_func_27(void)
{
  return -((((10))));
}
int
test_func_28(void)
{
  return 1 + 2;
}
int
test_func_29(void)
{
  return 1 - 2 - 3;
}
int
test_func_30(void)
{
  return 6 / 3 / 2;
}
int
test_func_31(void)
{
  return (3 / 2 * 4) + (5 - 4 + 3);
}
int
test_func_32(void)
{
  return 5 * 4 / 2 - 3 % (2 + 1);
}
int
test_func_33(void)
{
  return 4 / 2;
}
int
test_func_34(void)
{
  return (-12) / 5;
}
int
test_func_35(void)
{
  return 4 % 2;
}
int
test_func_36(void)
{
  return 2 * 3;
}
int
test_func_37(void)
{
  return 2 * (3 + 4);
}
int
test_func_38(void)
{
  return 2 + 3 * 4;
}
int
test_func_39(void)
{
  return 1 - 2;
}
int
test_func_40(void)
{
  return 2 - -1;
}
int
test_func_41(void)
{
  return ~2 + 3;
}
int
test_func_42(void)
{
  return ~(1 + 1);
}

int
test_func_47(void)
{
  return ~2 * -2 == 1 + 5;
}
int
test_func_48(void)
{
  return 1 == 2;
}

int
test_func_49(void)
{
  return 3 == 1 != 2;
}
int
test_func_50(void)
{
  return 1 == 1;
}
int
test_func_51(void)
{
  return 1 >= 2;
}
int
test_func_52(void)
{
  return (1 >= 1) + (1 >= -4);
}
int
test_func_53(void)
{
  return (1 > 2) + (1 > 1);
}
int
test_func_54(void)
{
  return 15 > 10;
}
int
test_func_55(void)
{
  return 1 <= -1;
}
int
test_func_56(void)
{
  return (0 <= 2) + (0 <= 0);
}
int
test_func_57(void)
{
  return 2 < 1;
}
int
test_func_58(void)
{
  return 1 < 2;
}

int
test_func_59(void)
{
  return 0 || 0 && (1 / 0);
}
int
test_func_60(void)
{
  return 0 != 0;
}
int
test_func_61(void)
{
  return -1 != -2;
}
int
test_func_62(void)
{
  return !-3;
}
int
test_func_63(void)
{
  return !5;
}
int
test_func_64(void)
{
  return !(4 - 4);
}
int
test_func_65(void)
{
  return !(3 - 44);
}
int
test_func_66(void)
{
  return !0;
}

int
test_func_74(void)
{
  return 2 == 2 || 0;
}

int
test_func_75(void)
{
  return (0 == 0 && 3 == 2 + 1 > 1) + 1;
}
int
test_func_76(void)
{
  int first_variable  = 1;
  int second_variable = 2;
  return first_variable + second_variable;
}
int
test_func_77(void)
{
  int a = 2147483646;
  int b = 0;
  int c = a / 6 + !b;
  return c * 2 == a - 1431655762;
}
int
test_func_78(void)
{
  int var0;
  var0 = 2;
  return var0;
}

int
test_func_79(void)
{
  int a = a = 5;
  return a;
}

int
test_func_80(void)
{
  int a;
  int b = a = 0;
  return b;
}

int
test_func_83(void)
{
  int a = -2593;
  a     = a % 3;
  int b = -a;
  return b;
}

int
test_func_84(void)
{
  int return_val = 3;
  int void2      = 2;
  return return_val + void2;
}
int
test_func_85(void)
{
  int a = 3;
  a     = a + 5;
  return a;
}
int
test_func_86(void)
{
  int a = 1;
  int b = 0;
  a     = 3 * (b = a);
  return a + b;
}

int
test_func_87(void)
{
  int a = 0;
  0 || (a = 1);
  return a;
}
int
test_func_88(void)
{
  return 42;
}
int
test_func_90(void)
{
  int a = 2;
  return a;
}

int
test_func_91(void)
{
  int a = 0;
  0 && (a = 5);
  return a;
}

int
test_func_92(void)
{
  int a = 0;
  1 || (a = 1);
  return a;
}
int
test_func_94(void)
{
  int a    = 1;
  int b    = 2;
  return a = b = 4;
}

int
test_func_95(void)
{
  int a = 0 && a;
  return a;
}
int
test_func_96(void)
{
  int a = 0;
  a     = 1 ? 2 : 3;
  return a;
}
int
test_func_97(void)
{
  if (1 + 2 == 3)
    return 5;
}
int
test_func_98(void)
{
  if (1 + 2 == 4)
    return 5;
  return 42;
}
int
test_func_99(void)
{
  int a = 0;
  if (a)
    return 1;
  else
    return 2;
}
int
test_func_100(void)
{
  int a = 1;
  int b = 0;
  if (a)
    b = 1;
  else if (b)
    b = 2;
  return b;
}
int
test_func_101(void)
{
  int a = 0;
  int b = 1;
  if (a)
    b = 1;
  else if (~b)
    b = 2;
  return b;
}

int
test_func_102(void)
{
  int a = 0;
  if ((a = 1)) {
    if (a == 1) {
      a = 3;
    }
    else {
      a = 4;
    }
  }

  return a;
}

int
test_func_103(void)
{
  int a = 0;
  if (!a) {
    if (3 / 4) {
      a = 3;
    }
    else {
      a = 8 / 2;
    }
  }

  return a;
}
int
test_func_104(void)
{
  int a = 0;
  if (0)
    if (0)
      a = 3;
    else
      a = 4;
  else
    a = 1;

  return a;
}
int
test_func_105(void)
{
  int a = 0;
  int b = 0;
  if (a)
    b = 1;
  return b;
}
int
test_func_106(void)
{
  int x = 0;
  if (0)
    ;
  else
    x = 1;
  return x;
}
int
test_func_107(void)
{
  int a = 1;
  int b = 0;
  if (a)
    b = 1;
  return b;
}
int
test_func_108(void)
{
  int x = 10;
  int y = 0;
  y     = (x = 5) ? x : 2;
  return (x == 5 && y == 5);
}
int
test_func_109(void)
{
  int a = 0;
  int b = 0;

  if (a)
    a = 2;
  else
    a = 3;

  if (b)
    b = 4;
  else
    b = 5;

  return a + b;
}
int
test_func_110(void)
{
  int a    = 1;
  int b    = 2;
  int flag = 0;

  return a > b ? 5 : flag ? 6 : 7;
}
int
test_func_111(void)
{
  int a = 1 ? 2 ? 3 : 4 : 5;
  int b = 0 ? 2 ? 3 : 4 : 5;
  return a * b;
}
int
test_func_112(void)
{
  int flag = 1;
  int a    = 0;
  flag ? a = 1 : (a = 0);
  return a;
}
int
test_func_113(void)
{
  int a = 0;
  return a > -1 ? 4 : 5;
}
int
test_func_114(void)
{
  int a = 1;
  a != 2 ? a = 2 : 0;
  return a;
}
int
test_func_115(void)
{
  int a = 1 ? 3 % 2 : 4;
  return a;
}
int
test_func_118(void)
{
  int a = 1;
  int b = 0;
  a ? (b = 1) : (b = 2);
  return b;
}
int
test_func_119(void)
{
  int a = 0;
  int b = 0;
  a ? (b = 1) : (b = 2);
  return b;
}

int
test_func_120(void)
{
  int a = 3;
  {
    int a = a = 4;
    return a;
  }
}
int
test_func_121(void)
{
  int a = 3;
  {
    int a = a = 4;
  }
  return a;
}

int
test_func_122(void)
{
  int a;
  {
    int b = a = 1;
  }
  return a;
}
int
test_func_123(void)
{
  int ten = 10;
  {
  }
  int twenty = 10 * 2;
  {
    {
    }
  }
  return ten + twenty;
}
int
test_func_124(void)
{
  int a = 2;
  int b;
  {
    a     = -4;
    int a = 7;
    b     = a + 1;
  }
  return b == 8 && a == -4;
}

int
test_func_125(void)
{
  int a = 2;
  {
    int a = 1;
    return a;
  }
}

int
test_func_126(void)
{
  int x = 4;
  {
    int x;
  }
  return x;
}
int
test_func_127(void)
{
  int a = 0;
  {
    int b = 4;
    a     = b;
  }
  {
    int b = 2;
    a     = a - b;
  }
  return a;
}
int
test_func_128(void)
{
  int a = 0;
  if (a) {
    int b = 2;
    return b;
  }
  else {
    int c = 3;
    if (a < c) {
      return !a;
    }
    else {
      return 5;
    }
  }
  return a;
}

int
test_func_129(void)
{
  int a;
  int result;
  int a1 = 1;
  {
    int a  = 2;
    int a1 = 2;
    {
      int a;
      {
        int a;
        {
          int a;
          {
            int a;
            {
              int a;
              {
                int a;
                {
                  int a;
                  {
                    int a;
                    {
                      int a  = 20;
                      result = a;
                      {
                        int a;
                        a      = 5;
                        result = result + a;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    result = result + a1;
  }
  return result + a1;
}
int
test_func_130(void)
{
  int x;
  {
    x = 3;
  }
  {
    return x;
  }
}
int
test_func_131(void)
{
  int a = 10;
  int b = 20;
  for (b = -20; b < 0; b = b + 1) {
    a = a - 1;
    if (a <= 0)
      break;
  }

  return a == 0 && b == -11;
}

int
test_func_132(void)
{
  int a = 10;
  while ((a = 1))
    break;
  return a;
}

int
test_func_133(void)
{
  int sum = 0;
  int counter;
  for (int i = 0; i <= 10; i = i + 1) {
    counter = i;
    if (i % 2 == 0)
      continue;
    sum = sum + 1;
  }

  return sum == 5 && counter == 10;
}

int
test_func_134(void)
{
  int sum = 0;
  for (int i = 0; i < 10;) {
    i = i + 1;
    if (i % 2)
      continue;
    sum = sum + i;
  }
  return sum;
}

int
test_func_135(void)
{
  int a = 1;
  do {
    a = a * 2;
  } while (a < 11);

  return a;
}

int
test_func_136(void)
{
  int a = 10;
  do
    break;
  while ((a = 1));
  return a;
}

int
test_func_137(void)
{
  return 0;
  ;
  ;
}

int
test_func_138(void)
{
  int i = 2147483642;
  do
    ;
  while ((i = i - 5) >= 256);

  return i;
}

int
test_func_139(void)
{
  int a = 12345;
  int i;

  for (i = 5; i >= 0; i = i - 1)
    a = a / 3;

  return a;
}

int
test_func_140(void)
{
  for (int i = 400;; i = i - 100)
    if (i == 100)
      return 0;
}

int
test_func_141(void)
{
  int a = -2147483647;
  for (; a % 5 != 0;) {
    a = a + 1;
  }
  return a % 5 || a > 0;
}

int
test_func_142(void)
{
  int a = 0;

  for (int i = -100; i <= 0; i = i + 1)
    a = a + 1;
  return a;
}

int
test_func_143(void)
{
  int i = 0;
  int j = 0;
  int k = 1;
  for (int i = 100; i > 0; i = i - 1) {
    int j = i + k;
    k     = j;
  }

  return k == 101 && i == 0 && j == 0;
}

int
test_func_144(void)
{
  int shadow = 1;
  int acc    = 0;
  for (int shadow = 0; shadow < 10; shadow = shadow + 1) {
    acc = acc + shadow;
  }
  return acc == 45 && shadow == 1;
}

int
test_func_145(void)
{
  int i = 0;
  while (1) {
    i = i + 1;
    if (i > 10)
      break;
  }
  int j = 10;
  while (1) {
    j = j - 1;
    if (j < 0)
      break;
  }
  int result = j == -1 && i == 11;
  return result;
}

int
test_func_146(void)
{
  int x = 10;
  int y = 0;
  int z = 0;
  do {
    z = z + 1;
    if (x <= 0)
      continue;
    x = x - 1;
    if (y >= 10)
      continue;
    y = y + 1;
  } while (z != 50);
  return z == 50 && x == 0 && y == 10;
}

int
test_func_147(void)
{
  int ans = 0;
  for (int i = 0; i < 10; i = i + 1)
    for (int j = 0; j < 10; j = j + 1)
      if ((i / 2) * 2 == i)
        break;
      else
        ans = ans + i;
  return ans;
}

int
test_func_148(void)
{
  int x   = 5;
  int acc = 0;
  while (x >= 0) {
    int i = x;
    while (i <= 10) {
      i = i + 1;
      if (i % 2)
        continue;
      acc = acc + 1;
    }
    x = x - 1;
  }
  return acc;
}

int
test_func_149(void)
{
  int acc = 0;
  int x   = 100;
  while (x) {
    int y = 10;
    x     = x - y;
    while (y) {
      acc = acc + 1;
      y   = y - 1;
    }
  }
  return acc == 100 && x == 0;
}

int
test_func_150(void)
{
  int a = 0;
  for (;;) {
    a = a + 1;
    if (a > 3)
      break;
  }

  return a;
}

int
test_func_151(void)
{
  int a = 0;

  while (a < 5)
    a = a + 2;

  return a;
}
