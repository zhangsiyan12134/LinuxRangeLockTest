#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct ct_fl_t;

/* block list and wait list segments */
struct ct_fl_seg{
    struct ct_fl_seg *prev;
    struct ct_fl_seg *next;
    struct ct_fl_t *addr;
};
typedef struct ct_fl_seg ct_fl_seg;

/* File lock */
struct ct_fl_t {
	struct ct_fl_t *fl_prev;
    struct ct_fl_t *fl_next;   		/* single liked list to other locks on this file */
	struct ct_fl_seg fl_block; 		/* locks that is blocking this lock */
	struct ct_fl_seg fl_wait; 		/* locks that is waiting for this lock*/

    int fl_fd;    					/*  Which fd has this lock*/
	volatile int fl_lock;			/* lock variable*/
	unsigned char fl_type;			/* type of the current lock*/
	unsigned int fl_pid;
    unsigned int fl_start;          /* starting address of the range lock*/
    unsigned int fl_end;            /* ending address of the range lock*/
};
typedef struct ct_fl_t ct_fl_t;

ct_fl_t head;

int check_overlap(struct ct_fl_t *lock1, struct ct_fl_t *lock2){
    return ((lock1->fl_start <= lock2->fl_start) && (lock1->fl_end >= lock2->fl_start)) ||\
    ((lock2->fl_start <= lock1->fl_start) && (lock2->fl_end >= lock1->fl_start));
}

//add a new node to the lock list upon the request(combined into lock_acq below)
int ctfs_lock_list_add_node(off_t start, size_t n, int flag){
    ct_fl_t *temp, *tail, *last;
    tail = head.fl_next;   //get the head of the lock list
    temp = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    temp->fl_next = NULL;
    temp->fl_prev = NULL;
    temp->fl_start = start;
    temp->fl_end = start + n - 1;
    //TODO: get locklist here
    while(tail != NULL){
        //check if current list contains a lock that is not compatable
        if(check_overlap(tail, temp)){
            printf("Overlap found, abourt");
            //overlap found stop the process
            return -1;
        }
        last = tail;
        tail = tail->fl_next;    
    }
    temp->fl_prev = last;
    last->fl_next = temp;

    //TODO: release locklist here
    return 0;
}

void ctfs_lock_list_remove_node(ct_fl_t *node){
    ct_fl_t *prev, *next;
    //TODO: get locklist here
    prev = node->fl_prev;
    next = node->fl_next;
    prev->fl_next = next;

    if (next != NULL)
        next->fl_prev = prev;
    //TODO: release locklist here
    free(node);
    return;
}

void main(void) {
    ct_fl_t *node1, *node2, *node3, *temp;

    node3 = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    node2 = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    node1 = (ct_fl_t*)malloc(sizeof(ct_fl_t));

    node3->fl_next = NULL;
    node3->fl_prev = node2;
    node3->fl_start = 25;
    node3->fl_end = 49;

    node2->fl_next = node3;
    node2->fl_prev = node1;
    node2->fl_start = 40;
    node2->fl_end = 59;

    node1->fl_next = node2;
    node1->fl_prev = &head;
    node1->fl_start = 10;
    node1->fl_end = 29;

    head.fl_next = node1;
    head.fl_prev = NULL;
    head.fl_start = 0;
    head.fl_end = 0;


    ctfs_lock_list_add_node(0, 5, 1);

    temp = head.fl_next;
    int i = 0;
    while(temp != NULL){
        i++;
        printf("Node: %d, Range: %ld - %ld\n", i, temp->fl_start, temp->fl_end);
        temp = temp->fl_next;
    }

    ctfs_lock_list_remove_node(node2);
    temp = head.fl_next;
    i = 0;
    while(temp != NULL){
        i++;
        printf("Node: %d, Range: %ld - %ld\n", i, temp->fl_start, temp->fl_end);
        temp = temp->fl_next;
    }

}