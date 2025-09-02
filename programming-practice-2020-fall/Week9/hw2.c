#include <stdio.h>

int main(void){
  int A[3][3], B[3][3], C[3][3];

  for (int i=0; i<3; i++){
    scanf("%d %d %d", &A[i][0], &A[i][1], &A[i][2]);
  }
  for (int i=0; i<3; i++){
    scanf("%d %d %d", &B[i][0], &B[i][1], &B[i][2]);
  }

  for (int i=0; i<3; i++){
    for (int j=0; j<3; j++){
      C[i][j] = 0;
      for (int k=0; k<3; k++){
        C[i][j] += A[i][k] * B[k][j];
      }
      printf("%5d", C[i][j]);
    }
    printf("\n");
  }
  return 0;
}