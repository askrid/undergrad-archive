#include <stdio.h>
#include <math.h>

int main(void){
  int n = 0, bin = 0;
  int i = 0;

  scanf("%d", &n);

  do{
    bin += (n%2) * pow(10, i++);
    n /= 2;
  }while(n > 0);

  printf("%d\n", bin);
}