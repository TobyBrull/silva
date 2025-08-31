#include "test_suite.hpp"

namespace silva::lox {
  vector_t<test_chapter_t> test_suite()
  {
    vector_t<test_chapter_t> retval;
    {
      retval.push_back(test_chapter_t{.name = "basic"});
      auto& rv = retval.back().test_cases;
      rv.push_back(test_case_t{" print 42.0 ; print 12.0 ; ", "42\n12\n"});
      rv.push_back(test_case_t{" print - 42.0 ; ", "-42\n"});
      rv.push_back(test_case_t{" print 1 + 2 * 3 + 4 ; ", "11\n"});
      rv.push_back(test_case_t{" print ! true ; ", "false\n"});
      rv.push_back(test_case_t{" print ! ( 1 + 2 == 3 ) ; ", "false\n"});
      rv.push_back(test_case_t{" print 1 + 2 != 4 ; ", "true\n"});
      rv.push_back(test_case_t{" print 1 + 2 <= 2 ; ", "false\n"});
      rv.push_back(test_case_t{" print 1 + 2 <= 3 ; ", "true\n"});
      rv.push_back(test_case_t{" print 1 + 2 <= 4 ; ", "true\n"});
      rv.push_back(test_case_t{" print 'hello' + ' world' ; ", "hello world\n"});
      rv.push_back(test_case_t{" print ! ( 5 - 4 > 3 * 2 == ! none ) ; ", "true\n"});
      rv.push_back(test_case_t{" var abc = 42 ; print 100 + abc ;", "142\n"});
      rv.push_back(test_case_t{" var a = 3 ; var b = 5 ; a = 10 + b * a ; print a ;", "25\n"});
      rv.push_back(test_case_t{
          " var a = 10 ; { print a ; var a = 20 ; var b = 30 ; print a ; print b ; } print a ; ",
          "10\n20\n30\n10\n"});
      rv.push_back(
          test_case_t{" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ",
                      "true\n"});
      rv.push_back(
          test_case_t{" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } else { a = 'false' ; } print a ; ",
                      "false\n"});
      rv.push_back(test_case_t{" var a; if ( 1 + 2 < 4 ) { a = 'true' ; } print a ; ", "true\n"});
      rv.push_back(test_case_t{" var a; if ( 1 + 2 > 4 ) { a = 'true' ; } print a ; ", "none\n"});
      rv.push_back(test_case_t{" var a = ( 1 + 2 > 4 ) or 'test' ; print a ; ", "test\n"});
      rv.push_back(test_case_t{" var a = ( 1 + 2 < 4 ) or 'test' ; print a ; ", "true\n"});
      rv.push_back(test_case_t{" var a = ( 1 + 2 > 4 ) and 'test' ; print a ; ", "false\n"});
      rv.push_back(test_case_t{" var a = ( 1 + 2 < 4 ) and 'test' ; print a ; ", "test\n"});
      rv.push_back(test_case_t{
          R"(
        var sum = 0 ;
        var i = 0 ;
        while ( i <= 3 ) {
          sum = sum + i ;
          i = i + 1 ;
        }
        print sum ;
        for ( ; i < 10 ; i = i + 1 ) {
          sum = sum + i ;
        }
        print sum ;
      )",
          "6\n45\n"});
      rv.push_back(test_case_t{"print true or 'short-circuit-1';", "true\n"});
      rv.push_back(test_case_t{"print false or 'short-circuit-2';", "short-circuit-2\n"});
      rv.push_back(test_case_t{"print true and 'short-circuit-3';", "short-circuit-3\n"});
      rv.push_back(test_case_t{"print false and 'short-circuit-4';", "false\n"});
    }
    {
      retval.push_back(test_chapter_t{.name = "functions"});
      auto& rv = retval.back().test_cases;
      rv.push_back(test_case_t{
          R"(
        fun sayHi(first, last) {
          print 'Hi, ' + first + ' ' + last + '!';
          return;
        }
        sayHi('Dear', 'Reader');
        )",
          "Hi, Dear Reader!\n"});
      rv.push_back(test_case_t{
          R"(
        fun foo1(y) {
          fun printer(x) {
            print x;
          }
          printer(y * 10);
        }
        fun foo2(y) {
          print y * 100;
          return;
        }
        var i = 42;
        fun bar(x) {
          x = x + 10;
          foo1(x);
          foo2(x);
          return x;
        }
        print bar(i);
      )",
          "520\n5200\n52\n"});
      rv.push_back(test_case_t{
          R"(
        fun fib(n) {
          if (n < 2) return n;
          return fib(n - 1) + fib(n - 2);
        }
        for ( var i = 1; i <= 6 ; i = i + 1 ) {
          print fib(i);
        }
      )",
          "1\n1\n2\n3\n5\n8\n"});
      rv.push_back(test_case_t{
          R"(
        var a;
        a = 'global';
        {
          fun showA() {
            print a;
          }

          showA();
          var a = 'block';
          showA();
          print a;
        }
        print a;
      )",
          "global\nglobal\nblock\nglobal\n"});
      rv.push_back(test_case_t{" print chr(81); ", "Q\n"});
      rv.push_back(test_case_t{
          R"(
        fun foo(x) {
          var a = x;
          x = 123;
          return a;
        }
        var outer = 42;
        print foo(outer);
        print outer;
    )",
          "42\n42\n",
      });
    }
    {
      retval.push_back(test_chapter_t{.name = "closures"});
      auto& rv = retval.back().test_cases;
      rv.push_back(test_case_t{
          R"(
        fun makeCounter() {
          var i = 0;
          fun count() {
            i = i + 1;
            print i;
          }
          return count;
        }
        var counter = makeCounter();
        counter();
        counter();
        counter();
      )",
          "1\n2\n3\n",
      });
      rv.push_back(test_case_t{
          R"(
        fun aaa() {
          var temp;
          {
            var x = 11;
            var y = 22;
            fun bbb() {
              print y;
              print x;
            }
            temp = bbb;
          }
          return temp;
        }
        var rv = aaa();
        rv();
      )",
          "22\n11\n",
      });
      rv.push_back(test_case_t{
          R"(
        fun aaa() {
          var y = 123;
          fun bbb() {
            var temp;
            {
              var x = 42;
              fun ccc() {
                print x;
                print y;
              }
              temp = ccc;
            }
            temp();
          }
          return bbb;
        }
        var rv = aaa();
        rv();
      )",
          "42\n123\n",
      });
      rv.push_back(test_case_t{
          R"(
        fun aaa() {
          var xxx = 123;
          fun bbb() {
            fun ccc() {
              print xxx;
            }
            var xxx = 42;
            fun ddd() {
              print xxx;
            }
            ccc();
            ddd();
            xxx = 43;
            ccc();
            ddd();
            return ccc;
          }
          var inner = bbb();
          inner();
          return inner;
        }
        var rv = aaa();
        rv();
      )",
          "123\n42\n123\n43\n123\n123\n",
      });
      rv.push_back(test_case_t{
          R"(
        var x = 'global';
        fun outer() {
          var x = 'outer';
          fun inner() {
            print x;
          }
          inner();
        }
        outer();
      )",
          "outer\n",
      });
      rv.push_back(test_case_t{
          R"(
        fun outer() {
          var x = 'value';
          fun middle() {
            fun inner() {
              print x;
            }
            print 'create inner closure';
            return inner;
          }
          print 'return from outer';
          return middle;
        }
        var mid = outer();
        var in = mid();
        in();
    )",
          "return from outer\ncreate inner closure\nvalue\n",
      });
      rv.push_back(test_case_t{
          R"(
        var globalSet;
        var globalGet;
        fun main() {
          var a = 'initial';
          fun set() { a = 'updated'; }
          fun get() { print a; }
          globalSet = set;
          globalGet = get;
        }
        main();
        globalGet();
        globalSet();
        globalGet();
    )",
          "initial\nupdated\n",
      });
      rv.push_back(test_case_t{
          R"(
        fun make_counter() {
          var x = 0;
          fun retval() {
            x = x + 1;
            print x;
          }
          return retval;
        }
        var c1 = make_counter();
        var c2 = make_counter();
        c1();
        c1();
        c1();
        c2();
        c2();
        c1();
    )",
          "1\n2\n3\n1\n2\n4\n",
      });
    }
    {
      retval.push_back(test_chapter_t{.name = "classes"});
      auto& rv = retval.back().test_cases;
      rv.push_back(test_case_t{
          R"(
        {
          class Toast {}
          var toast = Toast();
          print toast.jam = 'grape';
        }

        class Pair {}
        var pair = Pair();
        pair.first = 1;
        pair.second = 2;
        print pair.first + pair.second;
    )",
          "grape\n3\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Foo {
          bar(first, second) {
            print 'bar with ' + first + ' and ' + second;
          }
          baz(x) {
            print 'baz with ' + x;
          }
        }
        var foo = Foo();
        foo.bar('aa', 'bb');
        var bound_method = foo.baz;
        bound_method('cc');
    )",
          "bar with aa and bb\nbaz with cc\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Person {
          sayName() {
            print this.name;
          }
        }
        var jane = Person();
        jane.name = 'Jane';
        var method = jane.sayName;
        method();
    )",
          "Jane\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Person {
          init(name_param) {
            this.name = name_param;
          }
          sayName() {
            print 'I am ' + this.name;
          }
        }
        Person('Jane').sayName();
    )",
          "I am Jane\n",
      });
      rv.push_back(test_case_t{
          R"(
        class A {
          foo() { print 'A foo'; }
          bar() { print 'A bar'; }
        }
        class B < A {
          foo() { print 'B foo'; }
        }
        class C < B {}
        var c = C();
        c.foo();
        c.bar();
    )",
          "B foo\nA bar\n",
      });
      rv.push_back(test_case_t{
          R"(
        class A {
          method() { print 'A method'; }
        }
        class B < A {
          method() { print 'B method'; }
          test() { super.method(); }
        }
        class C < B {}
        var c = C();
        c.method();
        c.test();
    )",
          "B method\nA method\n",
      });
      rv.push_back(test_case_t{
          R"(
        class D {
          foo() {
            print 'foo D';
          }
        }
        class E < D {
          method() {
            var closure = super.foo;
            closure();
          }
          foo() {
            print 'foo E';
          }
        }
        class F < E {
        }
        var f = F();
        f.method();
    )",
          "foo D\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Thing {
          init() {
            print 'init()';
            this.mode = 'start';
          }
          getCallback() {
            fun localFunction() {
              print this.mode;
            }
            return localFunction;
          }
        }
        var callback = Thing().getCallback();
        callback();
    )",
          "init()\nstart\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Number {
          init(x) {
            this.value = x;
          }
        }
        class Inc {
          init() {
            this.i = 0;
            this.value = Number(0);
          }

          inc() {
            this.i = this.i + 10;
            this.value = Number(this.i);
          }
        }
        var inc = Inc();
        inc.inc();
        inc.inc();
        print inc.value.value;
    )",
          "20\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Animal {
          makeSound () {
            print '...';
          }
        }
        class Cat < Animal {
          makeSound () {
            var mm = super.makeSound;
            mm();
            print 'meow';
          }
        }
        class Dog < Animal {
        }
        var cat = Cat();
        var dog = Dog();
        cat.makeSound();
        dog.makeSound();
    )",
          "...\nmeow\n...\n",
      });
      rv.push_back(test_case_t{
          R"(
        class A {
          method() {
            print 'A method: ' + this.value;
          }
        }
        class B < A {
          method() {
            print 'B method: ' + this.value;
          }
          test() {
            print 'in test(): ' + this.value;
            super.method();
          }
        }
        class C < B {
        }
        var c = C();
        c.value = 'ccc';
        c.test();
    )",
          "in test(): ccc\nA method: ccc\n",
      });
      rv.push_back(test_case_t{
          R"(
        class Foo {
        }
        var f1 = Foo();
        var f2 = Foo();
        print f1 == f2;
        print f1 == f1;
    )",
          "false\ntrue\n",
      });
    }
    {
      retval.push_back(test_chapter_t{.name = "errors"});
      auto& rv = retval.back().test_cases;
      rv.push_back(test_case_t{" return 1 + ( 42 + 'world' ) ; ",
                               test_error_t{{
                                   "type error evaluating expression",
                                   "while executing instruction",
                                   "42 + 'world'",
                                   "[test.lox:1:15]",
                               }}});
    }
    return retval;
  }
}
