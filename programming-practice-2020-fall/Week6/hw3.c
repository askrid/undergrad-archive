#include <stdio.h>
#include <math.h>

int main(void){
  int n;
  float std_x, std_y;
  float x, y;
  float dst, temp;
  float ans_x, ans_y;

  scanf("%d%f%f", &n, &std_x, &std_y);

  for (int i=0; i<n; i++){
    scanf("%f%f", &x, &y);

    temp = pow(x-std_x, 2) + pow(y-std_y, 2);

    if (dst > temp || !i){
      dst = temp;
      ans_x = x, ans_y = y;
    }
  }
  dst = pow(dst, 0.5);

  printf("The closest point from (%.2f, %.2f) is (%.2f, %.2f) and distance between them is %.2f\n", std_x, std_y, ans_x, ans_y, dst);

  return 0;
}