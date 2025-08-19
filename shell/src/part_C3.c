#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
#include <signal.h>
extern fg_job* current_fg_job;
int pipe_function(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, int *pipe_fd, vector_t *pids, vector_t *bg_job_list, bool should_log, pid_t *pgid)
{
    int rc = fork();
    // Set the group process id
    if (!rc)
    {
        setpgid(0, *pgid);

        close(pipe_fd[0]);               // Child doesn't read from this pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect child's output to the pipe
        close(pipe_fd[1]);

        // tcsetpgrp(STDIN_FILENO, getpgrp());
        // executing the command
        decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
        exit(0);
    }
    if (*pgid == 0)
        *pgid = rc;
    fg_job *copy = malloc(sizeof(fg_job));
    copy->pid = rc;

    char cmd_name[4097] = {0};
    for (int i = 0; i < (int)to_be_passed->size; i++)
    {
        char *cmd_part = strdup(((string_t *)to_be_passed->data)[i].data);
        strcat(cmd_name, cmd_part);
        char *l = strdup(" ");
        strcat(cmd_name, l);
    }
    strcat(cmd_name," | ");
    copy->command_name = malloc(4097*sizeof(char));
    strcpy(copy->command_name,cmd_name);
    if(!current_fg_job)
    {
        current_fg_job=copy;
    }
    else
    {
        strcat(current_fg_job->command_name,copy->command_name);
    }
    vector_push_back(pids, copy);   // Save the child's PID
    close(pipe_fd[1]);              // Parent doesn't write to this pipe
    dup2(pipe_fd[0], STDIN_FILENO); // Redirect parent's input from the pipe
    close(pipe_fd[0]);

    vector_clear(to_be_passed);
    return 0;
}
