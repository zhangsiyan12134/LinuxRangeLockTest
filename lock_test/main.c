#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>

#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct ct_fl_t;

pthread_spinlock_t lock_list_spin;

enum mode{O_RDONLY, O_WRONLY, O_RDWR};

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
	struct ct_fl_seg *fl_block; 		/* locks that is blocking this lock */
	struct ct_fl_seg *fl_wait; 		/* locks that is waiting for this lock*/

	volatile int fl_lock;			/* lock itself*/

	int fl_fd;    					/* Which fd has this lock */
	unsigned char fl_type;			/* type of the current lock: O_RDONLY, O_WRONLY, or O_RDWR */
	unsigned int fl_pid;
    unsigned int fl_start;          /* starting address of the range lock*/
    unsigned int fl_end;            /* ending address of the range lock*/
    struct ct_fl_t *node_id;        /* For Debug Only */
};
typedef struct ct_fl_t ct_fl_t;

ct_fl_t* head;

void ctfs_lock_add_blocking(ct_fl_t *current, ct_fl_t *node){
    /* add the conflicted node into the head of the blocking list of the current node*/
    ct_fl_seg *temp;
    temp = (ct_fl_seg*)malloc(sizeof(ct_fl_seg));
    temp->prev = NULL;
    temp->next = current->fl_block;
    temp->addr = node;

    if(current->fl_block != NULL){
        current->fl_block->prev = temp;
    }
    current->fl_block = temp;
}

void ctfs_lock_add_waiting(ct_fl_t *current, ct_fl_t *node){
    /*add the current node to the wait list head of the conflicted node*/
    ct_fl_seg *temp;
    temp = (ct_fl_seg*)malloc(sizeof(ct_fl_seg));
    temp->prev = NULL;
    temp->next = node->fl_wait;
    temp->addr = current;
    if(node->fl_wait != NULL){
        node->fl_wait->prev = temp;
    }
    node->fl_wait = temp;
}

void ctfs_lock_remove_blocking(ct_fl_t *current){
    /* remove the current node from others' blocking list*/
    assert(current != NULL);
    ct_fl_seg *temp, *temp1, *prev, *next;
    temp = current->fl_wait;
    while(temp != NULL){    //go through all node this is waiting for current node
        temp1 = temp->addr->fl_block;
        while(temp1 != NULL){   //go thorough the blocking list on other nodes to find itself
            //compare range and mode
            if((temp1->addr->fl_start == current->fl_start) && (temp1->addr->fl_end == current->fl_end) && (temp1->addr->fl_type == current->fl_type)){
                prev = temp1->prev;
                next = temp1->next;
                if(prev != NULL)
                    prev->next = next;
                else
                    temp->addr->fl_block = NULL;    //last one in the blocking list
                if(next != NULL)
                    next->prev = prev;
                free(temp1);
                break;
            }
            temp1 = temp1->next;
        }
        printf("\tNode %p removed from Node %p blocking list\n", current, temp);
        if(temp->next == NULL){ //last member in waiting list
            free(temp);
            current->fl_wait = NULL;
            break;
        }
        temp = temp->next;
        if(temp != NULL)    //if not the last member
            free(temp->prev);
    }
}


int check_overlap(struct ct_fl_t *node1, struct ct_fl_t *node2){
    /* check if two given range have conflicts */
    return ((node1->fl_start <= node2->fl_start) && (node1->fl_end >= node2->fl_start)) ||\
    ((node2->fl_start <= node1->fl_start) && (node2->fl_end >= node1->fl_start));
}

int check_access_conflict(struct ct_fl_t *node1, struct ct_fl_t *node2){
    /* check if two given file access mode have conflicts */
    return !((node1->fl_type == O_RDONLY) && (node2->fl_type == O_RDONLY));
}

ct_fl_t* ctfs_lock_list_add_node(int fd, off_t start, size_t n, int flag){
    /* add a new node to the lock list upon the request(combined into lock_acq below) */
    ct_fl_t *temp, *tail, *last;
    temp = (ct_fl_t*)malloc(sizeof(ct_fl_t));
    temp->fl_next = NULL;
    temp->fl_prev = NULL;
    temp->fl_block = NULL;
    temp->fl_wait = NULL;
    temp->fl_type = flag;
    temp->fl_fd = fd;
    temp->fl_start = start;
    temp->fl_end = start + n - 1;
    temp->node_id = temp;

    pthread_spin_lock(&lock_list_spin);

    if(head != NULL){
        tail = head;   //get the head of the lock list
        while(tail != NULL){
            //check if current list contains a lock that is not compatable
            if(check_overlap(tail, temp) && check_access_conflict(tail, temp)){
                ctfs_lock_add_blocking(temp, tail); //add the conflicted lock into blocking list
                printf("\tNode %p is blocking the Node %p\n", tail, temp);
                ctfs_lock_add_waiting(temp, tail); //add the new node to the waiting list of the conflicted node
                printf("\tNode %p is waiting the Node %p\n", tail, temp);
            }
            last = tail;
            tail = tail->fl_next;
        }
        temp->fl_prev = last;
        last->fl_next = temp;
    } else {
        head = temp;
    }
    printf("Node %p added, Range: %u - %u, mode: %d\n", temp, temp->fl_start, temp->fl_end, temp->fl_type);
    pthread_spin_unlock(&lock_list_spin);

    return temp;
}

void ctfs_lock_list_remove_node(ct_fl_t *node){
    /* remove a node from the lock list upon the request */
    assert(node != NULL);
    ct_fl_t *prev, *next;

    pthread_spin_lock(&lock_list_spin);
    prev = node->fl_prev;
    next = node->fl_next;
    if (prev == NULL){
        if(next == NULL)
            head = NULL;    //last one member in the lock list;
        else{
            head = next;    //delete the very first node in list
            next->fl_prev = NULL;
        }
    } else {
        prev->fl_next = next;
        if (next != NULL)
            next->fl_prev = prev;
    }

    ctfs_lock_remove_blocking(node);
    printf("Node %p removed, Range: %u - %u, mode: %d\n", node, node->fl_start, node->fl_end, node->fl_type);
    pthread_spin_unlock(&lock_list_spin);

    free(node);
}

void print_all_info(){
    ct_fl_t *temp1;
    ct_fl_seg *temp2;
    pthread_spin_lock(&lock_list_spin);
    temp1 = head;
    printf("***********************Current List***********************\n");
    while(temp1 != NULL){
        printf("Node: %p, Range: %u - %u, mode: %d ===>\n", temp1->node_id, temp1->fl_start, temp1->fl_end, temp1->fl_type);

        temp2 = temp1->fl_wait;
        while(temp2 != NULL){
            printf("\tWaiting by: Node %p, Range: %u - %u, mode: %d\n", temp2->addr->node_id, temp2->addr->fl_start, temp2->addr->fl_end, temp2->addr->fl_type);
            temp2 = temp2->next;
        }

        temp2 = temp1->fl_block;
        while(temp2 != NULL){
            printf("\tBlocked by: Node %p, Range: %u - %u, mode: %d\n", temp2->addr->node_id, temp2->addr->fl_start, temp2->addr->fl_end, temp2->addr->fl_type);
            temp2 = temp2->next;
        }
        printf("\n");
        temp1 = temp1->fl_next;
    }
    printf("**********************************************************\n");
    pthread_spin_unlock(&lock_list_spin);
}

void* request_sim1(){
    ct_fl_t *node1;
    node1 = ctfs_lock_list_add_node(10086, 5, 20, O_RDONLY);
    while(node1->fl_block != NULL){} //wait for blocker finshed
    sleep(0.25);
    ctfs_lock_list_remove_node(node1);
    pthread_exit(NULL);
}

void* request_sim2(){
    ct_fl_t *node2;
    node2 = ctfs_lock_list_add_node(10086, 0, 5, O_RDWR);
    while(node2->fl_block != NULL){} //wait for blocker finshed
    sleep(0.05);
    ctfs_lock_list_remove_node(node2);
    pthread_exit(NULL);
}

void* request_sim3(){
    ct_fl_t *node3;
    node3 = ctfs_lock_list_add_node(10086, 0, 10, O_RDWR); // mode conflicted with node1&2
    while(node3->fl_block != NULL){} //wait for blocker finshed
    sleep(0.1);
    ctfs_lock_list_remove_node(node3);
    pthread_exit(NULL);
}

void* request_sim4(){
    ct_fl_t *node4;
    node4 = ctfs_lock_list_add_node(10086, 30, 10, O_WRONLY);
    while(node4->fl_block != NULL){} //wait for blocker finshed
    sleep(0.1);
    ctfs_lock_list_remove_node(node4);
    pthread_exit(NULL);
}

void* request_sim5(){
    ct_fl_t *node5;
    node5 = ctfs_lock_list_add_node(10086, 20, 20, O_WRONLY);
    while(node5->fl_block != NULL){} //wait for blocker finshed
    sleep(0.2);
    ctfs_lock_list_remove_node(node5);
    pthread_exit(NULL);
}

int main(void) {
    //ct_fl_t *node4, *node3, *node2, *node1;

    pthread_spin_init(&lock_list_spin, 0);

    pthread_t thread1, thread2, thread3, thread4, thread5;

    pthread_create(&thread1, NULL, request_sim1, NULL);
    pthread_create(&thread2, NULL, request_sim2, NULL);
    pthread_create(&thread3, NULL, request_sim3, NULL);
    pthread_create(&thread4, NULL, request_sim4, NULL);
    pthread_create(&thread5, NULL, request_sim5, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);

    print_all_info();

    pthread_spin_destroy(&lock_list_spin);

    return 0;
}
