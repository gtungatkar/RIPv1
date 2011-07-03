
struct listnode {
  struct listnode *next;
};

typedef struct listnode listnode;
//listnode *first;

int list_insert(listnode *first, listnode *where, listnode *toappend);
listnode * list_next(listnode *current);
listnode * list_nextdelete(listnode *first, listnode *current);
int list_append(listnode *first, listnode *toappend);
int is_list_empty(listnode * list);
int list_exist(listnode * list);
int list_delete(listnode *first, listnode *todelete);
