#include <stdio.h>

int main(void)
{
  int a = 0; int b = 0;
  
  scanf("%d%d", &a, &b);

  int b_p1 = b % 10;
  int b_p2 = b / 10;

  printf("%d\n%d\n%d\n", a*b_p1, a*b_p2, a*b_p2*10 + a*b_p1);

  return 0;
}