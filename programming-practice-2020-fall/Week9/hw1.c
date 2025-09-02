#include <stdio.h>
#include <string.h>

int main(void){
  char s[100];

  printf("input string : ");
  scanf("%s", s);

  int n = strlen(s);

  for (int i=0; i<n/2; i++){
    if (s[i] != s[n-1-i]){
      printf("%s is not a symmetrical word\n", s);
      return 0;
    }
  }
  printf("%s is a symmetrical word\n", s);
  return 0;
}