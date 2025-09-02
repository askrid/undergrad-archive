#include <stdio.h>
#include <math.h>

int count_sosu(int);

int main(void){
  int n = 0;

  scanf("%d", &n);

  printf("Count : %d\n", count_sosu(n));

  return 0;
}

int count_sosu(int n){
  int sosu[101] = {0,};
  int m = floor(sqrt(n));
  int cnt = 0;

  sosu[0] = 1;
  sosu[1] = 1;
 
  for (int i=2; i<=n; i++){
    if (1 - sosu[i]){
      printf("%d is sosu\n", i);
      cnt++;
      if (i <= m){
        for (int j=2; j<=n/i; j++)
          sosu[i*j] = 1;
      }
    }
  }
  return cnt;
}