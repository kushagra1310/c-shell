#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char* argv[])
{
    int fd;
    char buf[100];

    uint64 before=getreadcount();
    printf("Initial readcount: %lu\n",before);
    fd=open("../README",0); // 0= O_RDONLY in xv6

    if(fd<0)
    {
        printf("cannot open file\n");
        exit(1);
    }

    int n=read(fd,buf,100);
    if(n>0)
    printf("After readcount: %lu\n", getreadcount());
    return 0;
}