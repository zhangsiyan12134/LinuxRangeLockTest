#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct s {
        char m1;
        char m2;
};

int main(){
    printf("%d\n", &((struct s*)0)->m1);    //print of the offset of the selected member
    printf("%d\n", &((struct s*)0)->m2);
}