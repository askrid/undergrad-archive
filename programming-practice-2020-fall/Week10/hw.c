/*
* Programming Practice week 10
* Singly Linked List 
*/
#include <stdio.h>
#include <stdlib.h>

typedef struct __node{
  int data;
  struct __node* next;
}node;

typedef struct __list{
  node* head;
  int cnt;
}list;

void clear_list(list*);
void append_node(list*);
void insert_node(list*);  
void delete_node(list*);
void print_list(list*);
void reverse_list(list*); //hw1
void sort_list(list*, int);    //hw2
void head_to_tail(list*, list*);

int main(){
  printf("\033[2J\033[H"); //clear screen
  printf("\t**week10 practice**\n");
  /* init a list */
  list* L = (list*)malloc(sizeof(list));
  if(!L) printf("Failed to Init.\n");
  L->head = NULL;
  L->cnt = 0;
  while(1){
    printf("a : append   i : insert  d : delete\nr : reverse  s : sort    p : print\nq : quit\n");
    printf("Press key : ");    
    char c = getchar();
    getchar(); // remove '\n'   
    printf("\033[2J\033[H"); //clear screen   
    switch(c){
      case 'a' : append_node(L);  break;
      case 'i' : insert_node(L);  break;
      case 'd' : delete_node(L);  break;
      case 'r' : reverse_list(L); break;
      case 's' : sort_list(L, L->cnt);    break;
      case 'p' : print_list(L);   break;
      case 'q' : clear_list(L);   return 0;
      default : printf("Invalid Key\n");
    }
  }
  return 0;
}

void clear_list(list* L){
  while(L->head){
    node* tmp = L->head;
    L->head = L->head->next;
    free(tmp);
  }
  free(L);
}

void append_node(list* L){
  node* N = (node*)malloc(sizeof(node));
  if(!N){
    printf("Failed to create a node\n");
    return;
  }
  int n;
  printf("Data : ");
  scanf("%d", &n);
  getchar(); // remove '\n'
  N->data = n;
  N->next = L->head;
  L->head = N;
  L->cnt++;
  printf("\033[2J\033[H"); //clear screen
  printf("\t Append succeeded\n");   
}

void insert_node(list* L){
  int idx, n;
  node* N = (node*)malloc(sizeof(node));
  if(!N){
    printf("Failed to create a node\n");
    return;
  }
  printf("Index(0~) : ");
  scanf("%d", &idx);
  getchar(); // remove '\n'
  printf("Data : ");
  scanf("%d", &n);
  getchar();
  N->data = n;
  node* curr = L->head;
  node* prev = NULL;
  printf("\033[2J\033[H"); // clear screen
  if(idx > L->cnt || idx < 0){
    printf("Invalid Index\n");
    return;
  }
  else if(idx == 0){
    N->next = L->head;
    L->head = N;
  }
  else{
    while(idx--){
      prev = curr;
      curr = curr->next;
    }
    prev->next = N;
    N->next = curr;
  }
  L->cnt++;
  printf("\t Insert succeeded\n");
}

void delete_node(list* L){
  if(L->cnt == 0){
    printf("Empty\n");
    return;
  }
  int idx;
  node* curr = L->head;
  node* prev = NULL;
  printf("Index(0~) : ");
  scanf("%d", &idx);
  getchar(); // remove '\n'
  printf("\033[2J\033[H"); // clear screen
  if(idx > L->cnt-1 || idx < 0){
    printf("Invalid Index\n");
    return;
  }
  else if(idx == 0){
    L->head = L->head->next;
    free(curr);
  }
  else{    
    while(idx--){
      prev = curr;
      curr = curr->next;
    }
    prev->next = curr->next;
    free(curr);
  }
  L->cnt--;
  printf("\t Delete succeeded\n");
}

void print_list(list* L){
  if(L->cnt == 0){
    printf("Empty\n");
    return;
  }
  node* t = L->head;
  while(t){
    printf("%d ", t->data);
    t = t->next;
  }
  printf("\n");
}

void reverse_list(list* L){
  if(L->cnt < 2)
    return;
  node* cur = L->head;
  node* nxt = cur->next;
  node* tmp = NULL;
  cur->next = NULL;
  while(nxt->next){
    tmp = cur;
    cur = nxt;
    nxt = nxt->next;
    cur->next = tmp;
  }
  nxt->next = cur;
  L->head = nxt;
  printf("\t Reverse succeded\n");
}

void sort_list(list* L, int n){
  if(L->cnt < 2)
    return;
  int i = L->cnt / 2;
  list* L1 = (list*)malloc(sizeof(list));
  list* L2 = (list*)malloc(sizeof(list));
  L1->head = L->head;
  L1->cnt = 0;
  L2->head = L->head;
  L2->cnt = L->cnt;
  L->head = NULL;
  L->cnt = 0;
  while(i--){
    if(i){
      L2->head = L2->head->next;
    }
    else{
      node* tmp = L2->head->next;
      L2->head->next = NULL;
      L2->head = tmp;
    }
    L1->cnt++;
    L2->cnt--;
  }
  sort_list(L1, n); sort_list(L2, n);
  while(L1->head && L2->head){
    if(L1->head->data < L2->head->data){
      head_to_tail(L1, L);
    }
    else{
      head_to_tail(L2, L);
    }
  }
  while(L1->head || L2->head){
    head_to_tail(L1, L);
    head_to_tail(L2, L);
  }
  free(L1);
  free(L2);
  if(n == L->cnt)
    printf("\t Sort succeded\n");
}

void head_to_tail(list* M, list* N){
  if (!M->cnt)
    return;
  if (!N->cnt){
    N->head = M->head;
    node* tmp = M->head->next;
    M->head->next = NULL;
    M->head = tmp;
  }
  else{
    node* tail = N->head;
    for(int i=0; i<N->cnt-1; i++){
      tail = tail->next;
    }
    tail->next = M->head;
    node* tmp = M->head->next;
    M->head->next = NULL;
    M->head = tmp;
  }
  M->cnt--;
  N->cnt++;
}