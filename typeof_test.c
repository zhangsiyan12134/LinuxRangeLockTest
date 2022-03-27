#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void){
    int x = 5;
    typeof(x) y = 6;    //y has a int type like x
    printf("%d, %d\n", x, y);
}