import timeit

def fibonacci(n):
    f = 0
    s = 1
    for i in range(0, n - 1, 2):
        f += s
        s += f
    return f if n & 1 == 0 else s

def rangeFib():
    for i in range(95):
        f = fibonacci(i)
        print(i, f, hex(f), f < 0x8000000000000000)

print(timeit.timeit("fibonacci(50)", number=100000, globals=globals()))
print(timeit.timeit("fibonacci(200)", number=100000, globals=globals()))
print(timeit.timeit("fibonacci(1000)", number=100000, globals=globals()))
#rangeFib()