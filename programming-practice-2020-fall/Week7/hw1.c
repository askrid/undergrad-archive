#include <stdio.h>

int read_and_calc(void);

int main(void){
  printf("result: %d\n", read_and_calc());

  return 0;
}

int read_and_calc(void){
  char exp[99] = {0,};
  int n = 0;

  scanf("%s", exp);
  
  if (exp[0]) n += exp[0] - '0';

  for (int i=1; i<50; i++){
    if (exp[2*i-1] ==  '+')
      n += exp[2*i] - '0';
    else if (exp[2*i-1] == '-')
      n -= exp[2*i] - '0';
    else
      break;
  }
  return n;
}