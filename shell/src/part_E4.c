#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/headerfiles.h"
#include "../include/shell.h"
#include <signal.h>
extern pid_t foreground_pgid;
extern int bg_job_no;
extern fg_job *current_fg_job;

bg_job *find_job_by_number(vector_t *bg_job_list, int job_num)
{
    for (int i = 0; i < (int)bg_job_list->size; i++)
    {
        bg_job *job = &(((bg_job *)bg_job_list->data)[i]);
        if (job->job_number == job_num)
        {
            return job;
        }
    }
    return NULL;
}

int find_job_by_number_num(vector_t *bg_job_list, int job_num)
{
    for (int i = 0; i < (int)bg_job_list->size; i++)
    {
        bg_job *job = &(((bg_job *)bg_job_list->data)[i]);
        if (job->job_number == job_num)
        {
            return i;
        }
    }
    return -1;
}

int fg(vector_t *bg_job_list, int job_number)
{
    if (job_number > 0)
    {
        bg_job *to_be_brought = find_job_by_number(bg_job_list, job_number);
        if (!to_be_brought)
        {
            printf("No such job\n");
            return 0;
        }
        if (strcmp(to_be_brought->state, "Stopped") == 0)
        {
            kill(-(to_be_brought->pid), SIGCONT);
        }
        int status;
        int index = find_job_by_number_num(bg_job_list, job_number);
        current_fg_job = malloc(sizeof(fg_job));
        current_fg_job->pid = to_be_brought->pid;
        current_fg_job->command_name = strdup(to_be_brought->command_name);
        vector_erase(bg_job_list, index, to_be_brought);
        printf("%s\n", current_fg_job->command_name);
        waitpid(to_be_brought->pid, &status, 0);

        if (current_fg_job != NULL)
        {
            free(current_fg_job->command_name);
            free(current_fg_job);
            current_fg_job = NULL;
        }
    }
    return 0;
}
int bg(vector_t* bg_job_list, int job_number)
{
    if (job_number > 0)
    {
        bg_job *to_be_brought = find_job_by_number(bg_job_list, job_number);
        if (!to_be_brought)
        {
            printf("No such job\n");
            return 0;
        }
        if (strcmp(to_be_brought->state, "Stopped") == 0)
        {
            printf("[%d] %s &\n",to_be_brought->job_number,to_be_brought->command_name);
            kill(-(to_be_brought->pid), SIGCONT);
            free(to_be_brought->state);
            to_be_brought->state=strdup("Running");
        }
        else
        {
            printf("Job already running\n");
        }
    }
    return 0;
}
