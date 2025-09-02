import pandas as pd
import numpy as np
from scipy import stats

data = [[2.41*0.15, 2.68*0.15, 2.57*0.15,  2.78*0.15, 3.09*0.15],[2.48*0.17, 3.03*0.17, 2.42*0.17, 2.15*0.17, 2.15*0.17]]
data = pd.DataFrame(data)
data = data.transpose()

print(data.describe())

print(stats.levene(data[0], data[1]))
print(stats.ttest_ind(data[0], data[1], equal_var=True))

ans = int(input())
num = int(input())
while (num != ans):
    if (num > ans):
        print("너무 커욧")
    else:
        print("작아..")
    num = int(input())
print("딱 맞노")