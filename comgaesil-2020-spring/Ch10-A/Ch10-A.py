def f1(l):
	if not l: return 0
	return l[-1]+f1(l[:-1])

def f2(n):
    if n == 1:
        return 1
    elif n%2 == 1:
        return 1 + f2(3*n + 1)
    elif n%2 == 0:
        return 1 + f2(n // 2)

def f3(lst):
    if lst:
        print(lst[-1])
        f3(lst[:-1])

def f4(lst):
    if lst:
        print(3*lst[0]) if lst[0]%2 == 1 else None
        f4(lst[1:])

def f5(lst):
    if lst and lst[-1]%2 == 1:
        print(lst[-1]*3)
        f5(lst[:-1])
    elif lst and lst[-1]%2 == 0:
        print(lst[-1])
        f5(lst[:-1])

def f6(lst):
    if not lst:
        return []
    if type(lst[0]) == list:
        return f6(lst[0]) + f6(lst[1:])
    else:
        return [lst[0]] + f6(lst[1:])

def f7(n):
    if n==0:
        return 2
    elif n==1:
        return 1
    else:
        return f7(n-1)+f7(n-2)

def f8(s):
    if not s:
        return True
    s = s.lower()
    if s[0] == s[-1] and s[0].isalpha():
        return f8(s[1:-1])
    elif not s[0].isalpha():
        return f8(s[1:])
    elif not s[-1].isalpha():
        return f8(s[:-1])
    else:
        return False
        
def f9(n):
	if(n==0): return 1
	return n*f9(n-1)

def f10(lst):
    if not lst:
        return 0
    else:
        return 1 + f10(lst[:-1])

def f11(lst):
    if lst:
        return lst[0] if len(lst) == 1 else f11(lst[1:])

def f12(n):
	if(n==0): return
	print(n)
	f12(n-1)

def f13(n):
    if n < 10:
        return 1
    else:
        return 1 + f13(n//10)

def f14(lst):
    if lst:
        return lst[0] if lst[0]%2 == 1 else f14(lst[1:])

def f15(lst):
    if not lst: return 0
    return lst[0] + f15(lst[1:]) if lst[0]%2 == 1 else f15(lst[1:])

def f16(lst):
    if lst:
        return [lst[0]] + f16(lst[1:]) if lst[0]%2==1 else f16(lst[1:])
    else: return []

def f17(l):
	if len(l)==2:
		return l[0]
	else:
		return f17(l[1:])

def f18(a,b):
    if b == 0:
        return a
    else:
        return f18(b, a%b)

def f19(lst1, lst2):
    if not(lst1 and lst2):
        return lst1 + lst2
    elif lst1[0] <= lst2[0]:
        return [lst1[0]] + f19(lst1[1:], lst2)
    elif lst2[0] < lst1[0]:
        return [lst2[0]] + f19(lst1, lst2[1:])

def f20(lst):
    if len(lst)==0 or len(lst)==1:
        return lst
    else:
        half=len(lst)//2
        lst1=lst[:half]
        lst2=lst[half:]

        new1=f20(lst1)
        new2=f20(lst2)

        return f19(new1, new2)