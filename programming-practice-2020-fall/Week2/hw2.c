#include <stdio.h>

int main(void)
{
  float radius = 0.0;
  float pi = 3.14;

  printf("구의 반지름 값을 입력하세요.\n");
  scanf("%f", &radius);

  float volume = 4.0 / 3.0 * pi * radius * radius * radius;
  float surface_area = 4.0 * pi * radius * radius;

  printf("부피: %f, 겉넓이: %f", volume, surface_area);

  return 0;
}