#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"

int pipe_function(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, int* pipe_fd, vector_t* pids, vector_t* bg_job_list,bool should_log)
{
    int rc = fork();
    if (!rc)
    {                                   
        close(pipe_fd[0]);               // Child doesn't read from this pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect child's output to the pipe
        close(pipe_fd[1]);

        // Execute the command we've built so far
        decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list,bg_job_list, should_log);
        exit(0);
    }
    int *copy=malloc(sizeof(int));
    *copy=rc;
    vector_push_back(pids, copy);   // Save the child's PID
    close(pipe_fd[1]);              // Parent doesn't write to this pipe
    dup2(pipe_fd[0], STDIN_FILENO); // Redirect parent's input from the pipe
    close(pipe_fd[0]);

    vector_clear(to_be_passed);
    return 0;
}
