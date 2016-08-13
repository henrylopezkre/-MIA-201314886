#ifndef PARTS_H
#define PARTS_H
struct listitem {
    char data[10];
    struct listitem *next;
};

typedef struct listitem Listitem;

struct list {
    Listitem *head;
};

typedef struct list List;
void initlist (List *);
void insertfront(List * , char* val);
void insertback(List *, char* val);
int length(List);
void destroy(List *);
void setitem(List *, int n, char* val);
char* getitem(List, int n);

void initlist(List *ilist) {
    ilist->head = 0;
}

void insertfront(List *ilist, char* val) {
    Listitem *newitem;
    newitem = (Listitem *)malloc(sizeof(Listitem));
    newitem->next = ilist->head;
    strcpy(newitem->data, val);
    ilist->head = newitem;
}

void insertback(List *ilist, char* val) {
    Listitem *ptr;
    Listitem *newitem;

    newitem = (Listitem *)malloc(sizeof(Listitem));
    strcpy(newitem->data, val);
    newitem->next = 0;

    if (!ilist->head) {
        ilist->head = newitem;
        return;
    }

    ptr = ilist->head;
    while (ptr->next)
    {
        ptr = ptr->next;
    }
    ptr->next = newitem;
}
int length(List ilist){
    Listitem *ptr;
    int count = 1;

    if (!ilist.head) return 0;
    ptr = ilist.head;
    while (ptr->next) {
        ptr = ptr->next;
        count++;
    }
    return count;
}
void destroy(List *ilist) {
    Listitem *ptr1,
            *ptr2;
    if (!ilist->head) return;
    ptr1 = ilist->head;
    while (ptr1) {
        ptr2 = ptr1;
        ptr1 = ptr1->next;
        free(ptr2);
    }
    ilist->head = 0;
}
void setitem(List *ilist, int n, char* val){
    Listitem *ptr;
    int count = 0;

    if (!ilist->head) return;
    ptr = ilist->head;
    for (count = 0;count < n;count ++)
    {
        if (ptr) ptr = ptr->next;
        else return;
    }
    if (ptr)
        strcpy(ptr->data, val);
}

char* getitem(List ilist, int n) {
    Listitem *ptr;
    int count = 0;

    if (!ilist.head) return 0;
    ptr = ilist.head;
    if (n==0) return ptr->data;
    while (ptr->next) {
        ptr = ptr->next;
        count++;
        if (n == count)
            return (ptr->data);
    }
    return 0;
}

#endif // PARTS_H
