#include "../include/headerfiles.h"
#include "../include/shell.h"
int main()
{
    while(1)
    {
        display_prompt();
        char* inp=malloc(1025*sizeof(int));
        get_command(inp);
        if(!parse_shell_cmd(inp))
        printf("Invalid Syntax!");
        free(inp);
    }
    return 0;
}