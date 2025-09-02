def f1(lst):
    count = 0
    for n in lst:
        if n%2 == 1:
            count += 1
    return count

def f2(lst):
    for n in lst:
        if n%2 == 1:
            print(n)

def f3(lst):
    sum = 0
    for n in lst:
        if n%2 == 1:
            sum += n
    return sum

def f4(lst):
    sum = 0
    for n in range(len(lst)):
        if lst[n] % 2 == 1:
            sum += n
    return sum

def f5(lst):
    result = []
    for n in lst:
        result.append(n*n)
    return result

def f6(lst):
    max = lst[0]
    for n in lst:
        if n > max:
            max = n
    return max

def f7(lst):
    sum = 0.0
    for n in lst:
        sum += n
    return sum/len(lst)

def f8(a, b, n):
    for i in range(a,b+1):
        if i%n == 0:
            print(i)

def f9(width, height):
    line = '*'*width
    for i in range(height):
        print(line)

def f10(n):
    i = 1
    while i <= n:
        print('*'*i)
        i += 1

def f11(lst):
    for n in range(len(lst)-1):
        if lst[n] < lst[n+1]:
            return False
    return True

def f12(lst):
    for n in lst:
        if n >= 0:
            return False
    return True
        
def f13(lst, target):
    for n in range(len(lst)):
        if lst[n] == target:
            index = n
    return index

def f14(lst):
    for n in range(len(lst)):
        if lst[n] < 0:
            index = n
    return index

def f15(lst):
    sum = 0
    for n in range(0, len(lst), 2):
        sum += lst[n]
    return sum

def f16(n):
    while n >= 1:
        print('*'*n)
        n -= 1

def f17(lst):
    for n in range(len(lst)-1, -1, -2):
        print(lst[n])

def f18(n):
    factorial = 1
    for i in range(1, n+1):
        factorial *= i
    return factorial

def f19(lst):
    for n in lst:
        factorial = 1
        for i in range(1, n+1):
            factorial *= i
        print(factorial)

def f20(lst):
    for n in lst:
        for i in range(n, -1 ,-1):
            print(i,end = " \n"[i==0])

def f21(lst1, lst2):
    lst3 = []
    for n in range(len(lst1)):
        lst3.append(lst1[n]+lst2[n])
    return lst3

def f22(n):
    for i in range(1, n+1):
        if i%2==0 or i%3==0:
            print(i)

def f23(lst):
    max = lst[0][0]
    for sub_lst in lst:
        for n in sub_lst:
            if n > max:
                max = n
    return max
'''
def f24(lst):
    min = lst[0]
    for n in lst:
        if n < min:
            min = n
    max = second = min
    for n in lst:
        if n > max:
            second = max
            max = n
        elif n > second:
            second = n
    return second
'''
def f24(lst):
    fir, sec = lst[0], lst[1]
    if fir < sec:
        fir, sec = sec, fir
    for n in lst:
        if n > fir:
            fir, sec = n, fir
        elif fir > n > sec:
            sec = n
    print(sec)
 
def f25(n):
    while n >= 10:
        n //= 10
    return n

def f26(lst):
    for n in lst:
        max = n[0]
        for i in n:
            if i > max:
                max = i
        print(max)