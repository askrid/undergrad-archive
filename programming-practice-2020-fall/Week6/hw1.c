#include <stdio.h>
#include <math.h>

int main(void){
  int x1, y1, x2, y2, r1, r2;

  scanf("%d%d%d%d", &x1, &y1, &x2, &y2);
  scanf("%d%d", &r1, &r2);

  int dst = pow(x1-x2, 2) + pow(y1-y2, 2);
  int r_sum = pow(r1+r2, 2);
  int r_diff = pow(r1-r2, 2);

  int n = 2*(dst < r_sum && dst > r_diff) + (dst == r_sum || dst == r_diff);

  switch(n){
    case 0:
    printf("교점이 없습니다.\n"); break;
    case 1:
    printf("교점이 하나 입니다.\n"); break;
    case 2:
    printf("교점이 두 개 입니다.\n"); break;
    default:
    printf("error\n");
  }
  return 0;
}