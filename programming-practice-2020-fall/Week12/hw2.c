#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 20

typedef struct __node{
  int data;
  struct __node* next;
}node;

typedef struct __stack{
  node* top;
  int cnt;
}stack;

void push(stack*, int);
int pop(stack*);
void print_stack(stack*);

int main(){
  stack* stk = (stack*)malloc(sizeof(stack));
  if(!stk){
    printf("Failed to init.\n");
    return -1;
  }

  stk->cnt = 0;
  stk->top = NULL;
  
  push(stk, 1);
  push(stk, 3);
  push(stk, 5);
  push(stk, 7);
  print_stack(stk);

  free(stk);
  return 0;
}

void push(stack* stk, int n){
  if(stk->cnt == STACK_SIZE){
    printf("Stack is full\n");
    return;
  }

  node* N = (node*)malloc(sizeof(node));
  if(!N){
    printf("Failed to create a node.\n");
    return;
  }

  N->data = n;
  N->next = stk->top;
  stk->top = N;
  stk->cnt += 1;
}

int pop(stack* stk){
  if(stk->cnt == 0){
    printf("Stack is empty.\n");
    return 0;
  }

  node* tmp = stk->top;
  int tmp_n = tmp->data;
  stk->top = stk->top->next;
  stk->cnt -= 1;
  free(tmp);
  return tmp_n;
}

void print_stack(stack* stk){
  while(stk->cnt) printf("%d\n", pop(stk));
}