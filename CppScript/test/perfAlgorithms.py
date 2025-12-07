import timeit

def fibonacci(n):
    f = 0
    s = 1
    for i in range(0, n - 1, 2):
        f += s
        s += f
    return f if n & 1 == 0 else s

def factorial(n):
    r = n
    for i in range(2, n):
        r *= i
    return r


print("fibonacci(50)", fibonacci(50))
print(timeit.timeit("fibonacci(50)", number=10000, globals=globals()))
print("fibonacci(200)", fibonacci(200))
print(timeit.timeit("fibonacci(200)", number=10000, globals=globals()))
print("fibonacci(1000)", fibonacci(1000))
print(timeit.timeit("fibonacci(1000)", number=10000, globals=globals()))

print("factorial(20)", factorial(20))
print(timeit.timeit("factorial(20)", number=10000, globals=globals()))
print("factorial(50)", factorial(50))
print(timeit.timeit("factorial(50)", number=10000, globals=globals()))
print("factorial(100)", factorial(100))
print(timeit.timeit("factorial(100)", number=10000, globals=globals()))
print("factorial(200)", factorial(200))
print(timeit.timeit("factorial(200)", number=10000, globals=globals()))
