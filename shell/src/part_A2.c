#include "../include/headerfiles.h"
#include "../include/shell.h"
int get_command(char* buf)
{
    if(!fgets(buf,LINE_MAX,stdin))
    {
        perror("fgets failed");
        return 1;
    }
    //LLM used
    buf[strcspn(buf, "\n")] = '\0'; // replacing \n with \0, as fgets includes \n in it's string
    return 0;
}