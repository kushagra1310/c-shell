#include "../include/headerfiles.h"
#include "../include/shell.h"
#include <errno.h>
void hop_function(vector_t *token_list, char *home_dir, char *prev_dir)
{
    if (token_list->size == 1) // no arguments case (same as ~)
    {
        char cur_path[PATH_MAX + 1];
        if (getcwd(cur_path, sizeof(cur_path)) == NULL)
        {
            perror("getcwd failed");
            return;
        }
        if (chdir(home_dir) == 0)
        {
            strcpy(prev_dir, cur_path);
        }
        else
        {
            perror("chdir failed");
        }
        return;
    }

    for (int i = 1; i < (int)token_list->size; i++)
    {
        char cur_path[PATH_MAX];
        if (getcwd(cur_path, sizeof(cur_path)) == NULL)
        {
            perror("getcwd failed");
            continue;
        }
        int chdir_flag = 0;
        string_t temp = ((string_t *)token_list->data)[i];
        if (!strcmp(temp.data, "~"))
            chdir_flag = chdir(home_dir);
        else if (!strcmp(temp.data, ".."))
            chdir_flag = chdir("..");
        else if (!strcmp(temp.data, "-"))
        {
            if (prev_dir[0] == '\0')
            {
                // error check in case prev directory wasn't passed or updated correctly
                continue;
            }
            chdir_flag = chdir(prev_dir);
        }
        else if (!strcmp(temp.data, "."))
            continue;
        else
            chdir_flag = chdir(temp.data);

        if (chdir_flag)
        {
            if (errno == ENOENT)
                printf("No such directory!\n");
            else
                perror("chdir failed");
        }
        else
        {
            strcpy(prev_dir, cur_path); // update previous directory to current one for the next command
        }
    }
}
