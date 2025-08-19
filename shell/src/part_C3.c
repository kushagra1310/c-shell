#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
#include <signal.h>
int pipe_function(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, int *pipe_fd, vector_t *pids, vector_t *bg_job_list, bool should_log, pid_t* pgid)
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
    if(*pgid==0)
    *pgid=rc;
    int *copy = malloc(sizeof(int));
    *copy = rc;
    vector_push_back(pids, copy);   // Save the child's PID
    close(pipe_fd[1]);              // Parent doesn't write to this pipe
    dup2(pipe_fd[0], STDIN_FILENO); // Redirect parent's input from the pipe
    close(pipe_fd[0]);

    vector_clear(to_be_passed);
    return 0;
}
