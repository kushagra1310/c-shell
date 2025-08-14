#include "../include/headerfiles.h"
#include "../include/shell.h"
int main()
{
    char home_dir[]="/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell";
    char *prev_dir=malloc(4097*sizeof(char));
    strcpy(prev_dir,home_dir);
    
    while(1)
    {
        display_prompt(home_dir);
        char* inp=malloc(1025*sizeof(int));
        get_command(inp);
        if(!parse_shell_cmd(inp))
        printf("Invalid Syntax!");
        else
        {
            vector_t* token_list=tokenize_input(inp);
            if((int)token_list->size>0 && strcmp(((string_t*)token_list->data)[0].data,"hop")==0)
            {
                hop_function(token_list,home_dir,prev_dir);
            }
        }
        free(inp);
    }
    return 0;
}