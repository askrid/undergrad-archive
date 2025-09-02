#include <stdio.h>

unsigned bin_to_dec(unsigned);
void print_bin(unsigned);

int main(void){
  unsigned a, b;

  printf("Input 1 : ");
  scanf("%u", &a);
  printf("Input 2 : ");
  scanf("%u", &b);
  putchar('\n');

  a = bin_to_dec(a);
  b = bin_to_dec(b);

  printf("Result : ");
  print_bin(a+b);
  printf(" (%u)\n", a+b);

  return 0;
}

unsigned bin_to_dec(unsigned n){
  unsigned ret = 0, i = 1;
  while(n){
    ret += (n%10) * i;
    i *= 2;
    n /= 10;
  }
  return ret;
}

void print_bin(unsigned n){
  unsigned mask = 1 << 31;
  while(!(n&mask)){
    mask >>= 1;
  }
  while(mask){
    putchar(n&mask ? '1' : '0');
    mask >>= 1;
  }
}