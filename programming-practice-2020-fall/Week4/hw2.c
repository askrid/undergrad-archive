#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  for(int i = 0; i < 9; i++){
    int a = abs(i-4);
    for(int j = 0; j < (4-a); j++) printf(" ");
    for(int k = 0; k < (2*a+1); k++) printf("*");
    printf("\n");
  }

  return 0;
}