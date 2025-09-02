def integral(n):
    return sum([(lambda x: (pow(x, 5) - pow(x, 4)) * (10/n))((10/n) * i) for i in range(n)])

def pi(n):
    from random import uniform
    return sum([4 / n for i in range(n) if pow(uniform(-1,1), 2) + pow(uniform(-1,1), 2) <= 1])

print(integral(10000))