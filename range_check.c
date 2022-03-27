#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int check_overlap(struct flock *lock1, struct flock *lock2){
    return ((lock1->l_start <= lock2->l_start) && ((lock1->l_start + lock1->l_len - 1) >= lock2->l_start)) ||\
    ((lock2->l_start <= lock1->l_start) && ((lock2->l_start + lock2->l_len - 1) >= lock1->l_start));
}

int main(void){
   struct flock range1, range2, range3, range4;

    range1.l_type = F_RDLCK;
    range1.l_whence = SEEK_SET;
    range1.l_start = 10;        //byte 10-29
    range1.l_len = 20;
    range1.l_pid = -1;

    range2.l_type = F_RDLCK;
    range2.l_whence = SEEK_SET;
    range2.l_start = 20;        //byte 20-39 overlapped with range1
    range2.l_len = 20;
    range2.l_pid = -1;

    range3.l_type = F_RDLCK;
    range3.l_whence = SEEK_SET;
    range3.l_start = 40;        //byte 40-59 no overlap
    range3.l_len = 20;
    range3.l_pid = -1;

    range4.l_type = F_RDLCK;
    range4.l_whence = SEEK_SET;
    range4.l_start = 15;        //byte 15-19 overlapped with both range1 and 2
    range4.l_len = 15;
    range4.l_pid = -1;

    if(check_overlap(&range1, &range4)){
        printf("Overlapped!");
    }
    else{
        printf("No overlap!");
    }
    return 0;
}