#include <stdio.h>

int main(void)
{
  char c;
  int a = 0, num = 0, line = 0, space = 0;

  while((c = getchar()) != EOF){
    if (c == 'a') a++;
    if (c >= '0' && c <= '9') num++;
    if (c == '\n') line++;
    if (c == ' ') space++;
  }

  printf("a : %d\nnum : %d\nline : %d\nspace : %d\n", a, num, line, space);

  return 0;
}