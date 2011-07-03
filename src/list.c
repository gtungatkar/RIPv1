#include<stdlib.h>
#include<stdio.h>

#include "list.h"

int list_append(listnode *first, listnode *toappend)
{
  listnode *p;
  if(!list_exist(first)) {
     return -1;
  }
  p = first;
  while(p->next != NULL) {
    p = list_next(p);
  }
  p->next = toappend;
  toappend->next = NULL;
  return 0;
}
int list_insert(listnode *first, listnode *where, listnode *toappend)
{
  /* toappend is first node in list when where = NULL*/
  if(where == NULL) {
    where = toappend;
    toappend->next = first;
    first = where;
    first->next = NULL;
    return 0;
  }

  /* where is last node of list */
  if(where->next == NULL) {
    where->next = toappend;
    toappend->next = NULL;
    return 0;
  }

  /* where is any in-between node */
  toappend->next = where->next;
  where->next = toappend;
  return 0;
}

listnode * list_next(listnode *current)
{
  return (current->next);

}

/* returns 0 if node deleted. -1 if node not found in d list */

int list_delete(listnode *first, listnode *todelete)
{
  listnode *prev;
  if(list_exist(first)==0 || is_list_empty(first) ) {
    return -1;
  }
  if(todelete == NULL) {
    return -1;
  }
  prev = first;
  while(prev->next != todelete) {
    prev = prev->next;
    if(prev->next == NULL)
      return -1;
  }
  prev->next = todelete->next;
  todelete->next = NULL;
  return 0;
}

/* delete current->next and return the node */

listnode * list_nextdelete(listnode *first, listnode *current)
{
  listnode *temp;
  /* current = NULL when we want to delete first node */
  if(current == NULL) {
    temp = first;
    first = first->next;
    temp->next = NULL;
    return temp;
  }

  /*return null if current = last node */
  if(current->next == NULL)
    return NULL;

  temp = current->next;
  current->next = temp->next;
  temp->next = NULL;
  return temp;
}

int is_list_empty(listnode * list)
{
  if(list->next == NULL)
    return 1;
  return 0;
}
int list_exist(listnode * list)
{
  if(list == NULL)
    return 0;
  return 1;			/* return 1 if list exists */
}

/* UNIT TESTING Linked List */

/*
struct listdata {
  listnode *l;
  int data;
};
int main()
{
  struct listdata *new, *p, *q;
  new = (struct listdata *)malloc(sizeof(struct listdata));
  new->data = 10;
  first = NULL;
  list_insert(first, (listnode*)new);
  q = (struct listdata *)malloc(sizeof(struct listdata));
  q->data = 20;
  list_insert((listnode*)new,(listnode*)q); 
  list_nextdelete(NULL);
  p = (struct listdata*)first;
  do{
    printf("\n %d \n", p->data);
    p =(struct listdata *) list_next((listnode*)p); 
  }while(p!=NULL);
 return 0;
}
*/
