#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* File lock */
struct ct_fl_t {
    struct ct_fl_t *fl_next;   		/* single liked list to other locks on this file */
    struct ct_fl_t *fl_wait;        /* list of the accesses that is blocking this one*/
    struct ct_fl_t *fl_block;       /* list of the accesses that is waiting this one*/
    uint64_t fl_count;				/* how many read requests are received for this range*/
    int fl_fd;    					/*  Which fd has this lock*/
	volatile int fl_lock;			/* lock variable*/
    unsigned int fl_flags;
	unsigned char fl_type;			/* type of the current lock*/
	unsigned int fl_pid;
    unsigned int fl_start;          /* starting address of the range lock*/
    unsigned int fl_end;            /* ending address of the range lock*/
    struct hlist_head;
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
    temp->fl_count = 1;
    temp->fl_start = start;
    temp->fl_end = start + n - 1;
    temp->fl_flags = flag;
    //TODO: get lock list here
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
    last->fl_next = temp;
    //TODO: release locklist here
    return 0;
}

void main(void) {
    ct_fl_t *node1, *node2, *node3, *temp;

    node3 = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    node3->fl_next = NULL;
    node3->fl_count = 0;
    node3->fl_start = 25;
    node3->fl_end = 49;
    node3->fl_flags = 0;

    node2 = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    node2->fl_next = node3;
    node2->fl_count = 0;
    node2->fl_start = 40;
    node2->fl_end = 59;
    node2->fl_flags = 0;

    node1 = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    node1->fl_next = node2;
    node1->fl_count = 0;
    node1->fl_start = 10;
    node1->fl_end = 29;
    node1->fl_flags = 0;

    head.fl_next = node1;
    head.fl_count = 0;
    head.fl_start = 0;
    head.fl_end = 0;
    head.fl_flags = 0;


    ctfs_lock_list_add_node(0, 5, 1);

    temp = &head;
    int i = 0;
    while(temp != NULL){
        i++;
        printf("Node: %d, Range: %ld - %ld\n", i, temp->fl_start, temp->fl_end);
        temp = temp->fl_next;
    }

}