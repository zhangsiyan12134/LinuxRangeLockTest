//Reference https://radek.io/2012/11/10/magical-container_of-macro/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

struct list_head {
	struct list_head *next, *prev;
};

struct s {
        int id;
        struct list_head list1;
        struct list_head list2;
};


int main(){
    struct s *n1, *n2, *n3, *temp;
    n1 = (struct s *)malloc(sizeof(struct s));
    n2 = (struct s *)malloc(sizeof(struct s));
    n3 = (struct s *)malloc(sizeof(struct s));

    n1->id = 1;
    n2->id = 2;
    n3->id = 3;

    n1->list1.next = &n2->list1;
    n2->list1.next = &n3->list1;
    
    /*
    .-------------.     .-------------.     .-------------. 
    |     n1      |     |     n2      |     |     n3      |
    |-------------|     |-------------|     |-------------|
    | id     1    |     | id     2    |     | id     3    | 
    | list1->next | ==> | list1->next | ==> | list1->next |
    | list2       |     | list2       |     | list2       |
    '-------------'     '-------------'     '-------------'
    
    */

    temp = container_of(n2->list1.next, struct s, list1);

    printf("%d\n", temp->id);    //print of the index of the selected member
}