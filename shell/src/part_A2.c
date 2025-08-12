#include "../include/headerfiles.h"
#include "../include/shell.h"
void get_command(char* buf)
{
    if(!fgets(buf,1024,stdin))
    {
        perror("fgets failed");
        exit(1);
    }
}