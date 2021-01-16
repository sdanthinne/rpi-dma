#include "testTotal.h"
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main()
{

    char * string = "hello there is no problem";
    void * newMem;
    char * newplace;
    init_dma();
    //we need to force a flush of the cache
    usleep(1000);//hopefully this will be able to do some of the flush?
    newMem = videocore_allocator();
    strcpy(newMem,string);
    newplace = newMem+100;//move the pointer 100 
    printf("engine init\n");
    dmacp((void*)string,(void*)&newplace,strlen(string));
    usleep(1000);
    printf("moved value: '%s'\n",newplace);
    printf("original value: '%s'\n",newMem);
    if(string[0]==newplace[0])
    {
        printf("success\n");
    }else{
        printf("fail\n");
    }
    return 0;

}
