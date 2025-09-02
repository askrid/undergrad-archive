#include <stdio.h>

void print_bin(unsigned);
void print_hex(unsigned);

int main(void){
  unsigned n;

  printf("n: ");
  scanf("%u", &n);
  printf("<2진수>\n");
  print_bin(n);
  putchar('\n');
  printf("<16진수>\n");
  print_hex(n);

  return 0;
}

void print_bin(unsigned n){
  unsigned mask = 1 << 31;
  for(int i=0; i<32; i++){
    putchar(n&mask ? '1' : '0');
    mask >>= 1;
    if(i%8 == 7) putchar(' ');
  }
  putchar('\n');
}

void print_hex(unsigned n){
  char* HEX = "0123456789abcdef";
  unsigned mask = 0xF << 28;
  for(int i=7; i>-1; i--){
    putchar(HEX[(n&mask) >> (i*4)]);
    mask >>= 4;
  }
  putchar('\n');
}