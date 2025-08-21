#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
// LLM
int bg_job_comparator(const void *a, const void *b)
{
    const bg_job *job_a = (const bg_job *)a;
    const bg_job *job_b = (const bg_job *)b;
    return strcmp(job_a->command_name, job_b->command_name);
}
// LLM
int activity_function(vector_t *bg_job_list)
{   
    vector_t sorted_list;
    vector_init(&sorted_list, sizeof(bg_job), bg_job_list->size);
    memcpy(sorted_list.data, bg_job_list->data, bg_job_list->size * sizeof(bg_job));
    sorted_list.size = bg_job_list->size;
    vector_sort(&sorted_list, bg_job_comparator);
    for (int i = 0; i < (int)sorted_list.size; i++)
    {
        printf("[%d] : %s - %s\n", ((bg_job *)sorted_list.data)[i].pid, ((bg_job *)sorted_list.data)[i].command_name, ((bg_job *)sorted_list.data)[i].state);
    }
    vector_free(&sorted_list);
    return 0;
}
