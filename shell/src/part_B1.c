#include "../include/headerfiles.h"
#include "../include/shell.h"
void hop_function(vector_t *token_list, char *home_dir, char *prev_dir)
{
    for (int i = 1; i < (int)token_list->size; i++)
    {
        char cur_path[4097];
        getcwd(cur_path, sizeof(cur_path));
        int chdir_flag = 0;
        string_t temp = ((string_t *)token_list->data)[i];
        if (!strcmp(temp.data, "~") || !strcmp(temp.data, ""))
            chdir_flag = chdir(home_dir);
        else if (!strcmp(temp.data, ".."))
            chdir_flag = chdir("..");
        else if (!strcmp(temp.data, "-"))
            chdir_flag = chdir(prev_dir);
        else if (strcmp(temp.data, "."))
            chdir_flag = chdir(temp.data);
        if (chdir_flag)
            perror("chdir failed");
        else
        {
            strcpy(prev_dir, cur_path);
        }
    }
}
