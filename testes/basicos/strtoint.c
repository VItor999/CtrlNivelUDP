#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
int main()
{
    int a=500;
    char buffer[32];
    snprintf(buffer, 32, "%d", a);
    printf("Binary value = %s\n", buffer);

    return 0;
}