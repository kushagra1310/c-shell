#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/headerfiles.h"
#include "../include/shell.h"
int execute_cmd(char *inp, char *home_dir, char *prev_dir, Queue* log_list)
{
    vector_t *token_list = tokenize_input(inp);
    if ((int)token_list->size > 0 && strcmp(((string_t *)token_list->data)[0].data, "hop") == 0)
    {
        hop_function(token_list, home_dir, prev_dir);
    }
    else if ((int)token_list->size > 0 && strcmp(((string_t *)token_list->data)[0].data, "reveal") == 0)
    {
        reveal_function(token_list, home_dir, prev_dir);
    }
    log_function(token_list,inp,prev_dir,home_dir,log_list);
    return 0;
}
