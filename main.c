#include "dmacp.h"

int main()
{

    char * string = "hello there is no problem";
    char * newplace;
    dmacp(&string,sizeof(string));
    if(string[0]==newplace[0])
    {
        print("success");
    }
    return 0;

}
