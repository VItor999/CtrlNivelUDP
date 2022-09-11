#include <pthread.h>
#include <stdio.h>
#include <string.h>
typedef struct s_xyzzy
{
    int num;
    char name[20];
    float secret;
} xyzzy;

/* This is our thread function.  It is like main(), but for a thread*/
void *threadFunc(void *arg)
{
    char *str;
    int i = 0;

    str = ((xyzzy *)arg)->name;
    int aux= ((xyzzy *)arg)->num;
    printf("NUM %d",aux);

    while (i < 15)
    {
        usleep(1);
        printf("threadFunc says: %s\n", str);
        ++i;
    }

    return NULL;
}

int main(void)
{
    xyzzy plugh;
    plugh.num = 42;
    strcpy(plugh.name, "paxdiablo");
    plugh.secret = 3.141592653589;
    pthread_t pth; // this is our thread identifier
    int i = 0;
    int porta = 10;
    pthread_create(&pth, NULL, threadFunc, &plugh);

    while (i < 10)
    {
        usleep(1);
        printf("main is running...\n");
        ++i;
    }

    printf("main waiting for thread to terminate...\n");
    pthread_join(pth, NULL);

    return 0;
}