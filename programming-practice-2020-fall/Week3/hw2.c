#include <stdio.h>

int main(void)
{
  int n = 0;

  scanf("%d", &n);

  n = (n + 4) % 26;

  printf("%c\n", 97+n);

  return 0;
}