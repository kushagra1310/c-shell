#include "../include/headerfiles.h"
#include "../include/shell.h"
int get_command(char* buf)
{
    if(!fgets(buf,1024,stdin))
    {
        // perror("fgets failed");
        return 1;
    }
    //LLM used
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}