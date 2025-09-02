def f1(lst):
    return len(list(filter(lambda x: x % 2 == 1, lst)))

def f2(lst):
    list(map(print, list(filter(lambda x: x % 2 == 1, lst))))

def f3(lst):
    return sum(list(filter(lambda x: x % 2 == 1, lst)))
    
def f4(lst):
    return sum(list(filter(lambda x: lst[x] % 2 == 1, range(len(lst)))))

def f5(lst):
    return list(map(lambda x: x ** 2, lst))

def f6(lst):
    return max(lst)

def f7(lst):
    return sum(lst)/len(lst)

def f8(a, b, n):
    list(map(print, list(filter(lambda x: x % n == 0, range(a, b+1)))))

def f9(width, height):
    list(map(print, ['*' * width] * height))
    
def f10(n):
    list(map(lambda x: print('*' * x), range(1, n+1)))

def f11(lst):
    return not list(filter(lambda x: lst[x] < lst[x+1], range(len(lst)-1)))

def f12(lst):
    return not list(filter(lambda x: x >= 0, lst))

def f13(lst, target):
    return next(filter(lambda x: lst[x] == target, range(len(lst)-1, -1, -1)))

def f14(lst):
    return next(filter(lambda x: lst[x] < 0, range(len(lst)-1, -1, -1)))

def f15(lst):
    return sum(lst[::2])

def f16(n):
    list(map(lambda x: print('*' * x), range(n, 0, -1)))

def f17(lst):
    list(map(print, lst[::-2]))

def f18(n):
    return 1 if n == 0 else n*f18(n-1)

def f19(matrix):
    list(map(lambda x: print(sum(matrix[x])), range(len(matrix))))

def f20(matrix):
    list(map(lambda x: print(matrix[x][x]), range(len(matrix))))

def f21(lst):
    from functools import reduce
    list(map(lambda n: print(reduce(lambda x, y: x * y, range(1, n+1))), lst))

def f22(lst):
    list(map(lambda n: list(map(lambda x: print(x, end = ' \n'[x==0]), range(n, -1, -1))), lst))

def f23(lst1, lst2):
    return list(map(lambda x, y: x + y, lst1, lst2))

def f24(n):
    list(map(print, list(filter(lambda x: x % 2 == 0 or x % 3 == 0, range(1, n+1)))))

def f25(lst):
    return max(sum(lst, []))

def f25(lst):
    return max(list(map(lambda x: max(x), lst)))

def f26(lst):
    return sorted(lst)[-2]

def f27(n):
    return int(str(n)[0])

def f28(lst):
    list(map(lambda x: print(max(x)), lst))