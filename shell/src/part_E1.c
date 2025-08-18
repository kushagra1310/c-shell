#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
int activity_function(vector_t *bg_job_list)
{
    for (int i = 0; i < (int)bg_job_list->size; i++)
    {
        printf("[%d] : %s - %s\n", ((bg_job *)bg_job_list->data)[i].pid, ((bg_job *)bg_job_list->data)[i].command_name, ((bg_job *)bg_job_list->data)[i].state);
    }
    return 0;
}
