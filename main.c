#include "testTotal.h"
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

int main()
{

    char * string = "hello there is no problem";
    char newplace[50];
    //we need to force a flush of the cache
    usleep(1000);//hopefully this will be able to do some of the flush?
    dmacp((void*)string,(void*)&newplace,sizeof(string));
    if(string[0]==newplace[0])
    {
        print("success");
    }
    return 0;

}
