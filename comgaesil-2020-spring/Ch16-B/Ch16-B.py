def match_func(lst, n):
    return 0 if not lst else int(lst[0] == n) + match_func(lst[1:], n)

def twice_elem(lst):
    return [] if not lst else [lst[0], lst[0]] + twice_elem(lst[1:])

def check_sum(lst, n):
    return n == 0 if not lst else check_sum(lst[1:], n - lst[0])

def repeat_elem(lst):
    lst.sort()
    return [] if not lst else ([lst[0]] if lst.count(lst[0]) > 1 else []) + repeat_elem(lst[lst.count(lst[0]):])
