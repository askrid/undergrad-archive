#include <stdio.h>
#include <string.h>

void swap(char**, char**);
void bubble_sort(char* words[10]);

int main(void){
  char* words[10] = {
    "computer", "windows", "window", "linux",
    "apple", "banana", "time", "help",
    "game", "money"};
  
  bubble_sort(words);
  for(int i=0; i<10; i++){
    printf("%s\n", words[i]);
  }
}

void swap(char** a, char** b){
  char* t = *a;
  *a = *b;
  *b = t;
}

void bubble_sort(char* words[10]){
  for (int i=0; i<9; i++){
    for (int j=9; j>i; j--){
      const char* s1 = words[j], *s2 = words[j-1];
      if (strlen(s1) < strlen(s2))
        swap(words+j, words+j-1);
      else if (strlen(s1) == strlen(s2) && (strcmp(s1, s2) < 0))
        swap(words+j, words+j-1);
    }
  }
}