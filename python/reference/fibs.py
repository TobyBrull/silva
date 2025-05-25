#!/usr/bin/env python
import time


def fib(n):
    if n <= 1:
        return n
    else:
        return fib(n - 1) + fib(n - 2)


if __name__ == "__main__":
    start_time = time.time()
    for i in range(35):
        res = fib(i)
        print(f'{i:3} {res:10} {time.time() - start_time:10.4}')
