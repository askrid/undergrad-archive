#include <stdio.h>
#include <math.h>

int main(void)
{
  float score1, score2, score3;
  score1 = 0.0; score2 = 0.0; score3 = 0.0;

  printf("세 명의 학생의 점수를 입력하세요.\n");
  scanf("%f %f %f", &score1, &score2, &score3);

  float average = (score1 + score2 + score3) / 3.0;
  float variance = (pow((score1 - average), 2) + pow((score2 - average), 2) + pow((score3 - average), 2)) / 3.0;
  printf("평균: %f 분산: %f\n", average, variance);

  return 0;
}