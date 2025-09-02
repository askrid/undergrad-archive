def f1(n):
    for i in range(1, n+1):
        for num in range(1, i+1):
            print(num, end = " ")
        print()

def f2(n):
    num = 0
    for i in range(1, n+1):
        for j in range(i):
            num += 1
            print(num, end = " ")
        print()

def f3(n):
    for i in range(1, n+1):
        num = 1 + i*(i-1)//2
        for j in range(i):
            print(num, end = " ")
            num += 1
        print()
    for i in range(n-1,0,-1):
        num = 1 + i*(i-1)//2
        for j in range(i):
            print(num, end = " ")
            num += 1
        print()

def f3(n):
    ret = []
    num = 0
    for i in range(1, n+1):
        ret.append([])
        for j in range(i):
            num += 1
            ret[-1].append(num)
    for t in ret:
        for v in t:
            print(v, end = "")
        print()
    for t in ret[-2::-1]:
        for v in t:
            print(v, end = "")
        print()

def f4(n):
    num = 1
    for i in range(1,n+1):
        for j in range(i):
            print(num, end = " ")
            num += 1
        print()
    for i in range(n-1,0,-1):
        for j in range(i):
            print(num, end = " ")
            num += 1
        print()

def f5(matrix):
    for row in matrix:
        sum = 0
        for n in row:
            sum += n
        print(sum)

def f6(matrix):
    for n in range(len(matrix)):
        print(matrix[n][n])

def f7(matrix):
    for row in matrix:
        sum = 0
        for n in row:
            sum += n
        print(sum)

def f8(matrix):
    sum = 0
    for row in matrix:
        for n in row:
            sum += n
    return sum

def f9(matrix):
    product = 1
    for row in matrix:
        for n in row:
            product *= n
    return product

def f10(matrix):
    for row in matrix:
        for n in row:
            if n%2 == 1:
                print(n, end = " ")
        print()
                
def f11(matrix1, matrix2):
    result = []
    for row in range(len(matrix1)):
        result_row = []
        for col in range(len(matrix1[0])):
            result_row.append(matrix1[row][col] + matrix2[row][col])
        result.append(result_row)
    return result

def f12(matrix1, matrix2):
    result = []
    for row in range(len(matrix1)):
        result_row = []
        for col in range(len(matrix2[0])):
            product = 0
            for n in range(len(matrix1[0])):
                product += matrix1[row][n] * matrix2[n][col]
            result_row.append(product)
        result.append(result_row)
    return result

def f13(matrix):
    if len(matrix) == len(matrix[0]):
        isidentity = True
    else: isidentity = False
    for row in range(len(matrix)):
        for col in range(len(matrix)):
            if row == col and matrix[row][col] != 1:
                isidentity = False
            elif row != col and matrix[row][col] != 0:
                isidentity = False
    return isidentity 

def f14(rows, cols):
    result = []
    for row in range(rows):
        result_row = []
        for col in range(cols):
            n = 2 + (0 < col < cols-1) + (0 < row < rows-1)
            result_row.append(n)
        result.append(result_row)
    return result