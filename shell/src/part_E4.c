#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/headerfiles.h"
#include "../include/shell.h"
#include <signal.h>
extern pid_t foreground_pgid;
extern int bg_job_no;
extern fg_job *current_fg_job;
extern pid_t shell_pgid;

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
        // printf("[DEBUG] 'fg' initiated. Found job '%s' with stored PGID: %d\n", to_be_brought->command_name, to_be_brought->pid);


        // printf("[DEBUG] Giving terminal control to PGID %d...\n", to_be_b.rought->pid);
        tcsetpgrp(STDIN_FILENO, to_be_brought->pid);
        if (strcmp(to_be_brought->state, "Stopped") == 0)
        {
            kill(-(to_be_brought->pid), SIGCONT);
        }
        int status;
        int index = find_job_by_number_num(bg_job_list, job_number);
        current_fg_job = malloc(sizeof(fg_job));
        current_fg_job->pid = to_be_brought->pid;
        current_fg_job->command_name = strdup(to_be_brought->command_name);
        foreground_pgid = to_be_brought->pid;
        vector_erase(bg_job_list, index, NULL);
        printf("%s\n", current_fg_job->command_name);
        waitpid(to_be_brought->pid, &status, WUNTRACED);
        
        // printf("[DEBUG] Job finished or stopped. Taking back terminal control for shell (PGID %d)...\n", shell_pgid);
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        if (WIFSTOPPED(status))
        {
            printf("\n[%d] Stopped\t%s\n", bg_job_no, current_fg_job->command_name);
            
            // printf("[DEBUG] Job was STOPPED. Adding it back to background list.\n");
            bg_job stopped_job;
            stopped_job.pid = foreground_pgid;
            stopped_job.job_number = bg_job_no++;
            stopped_job.command_name = strdup(current_fg_job->command_name);
            stopped_job.state = strdup("Stopped");
            vector_push_back(bg_job_list, &stopped_job);
        }
        // else
        // printf("[DEBUG] Job TERMINATED.\n");
        if (current_fg_job != NULL)
        {
            free(current_fg_job->command_name);
            free(current_fg_job);
            current_fg_job = NULL;
        }
        foreground_pgid = -1;
    }
    return 0;
}
int bg(vector_t *bg_job_list, int job_number)
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
            printf("[%d] %s &\n", to_be_brought->job_number, to_be_brought->command_name);
            kill(-(to_be_brought->pid), SIGCONT);
            free(to_be_brought->state);
            to_be_brought->state = strdup("Running");
        }
        else
        {
            printf("Job already running\n");
        }
    }
    return 0;
}
