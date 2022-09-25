#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
    int i;
    srandom(time(NULL));   // Initialization, should only be called once.
    for(i=0; i<5; i++){
        printf("%d: %d\n",i,random()%100);
        sleep(1);
    }
    return 0;
}