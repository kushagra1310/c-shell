#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
#include <signal.h>
// just asked co pilot to add error checks for sys calls
extern fg_job *current_fg_job;

int pipe_function(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, int *pipe_fd, vector_t *pids, vector_t *bg_job_list, bool should_log, pid_t *pgid, fg_job **job_info, int file_input, int file_output)
{
    int rc = fork();
    if (rc < 0)
    {
        perror("fork failed");
        return -1;
    }

    // Set the group process id
    if (rc == 0)
    {
        if (setpgid(0, *pgid) < 0)
        {
            perror("setpgid failed");
            exit(1);
        }

        if (close(pipe_fd[0]) < 0) // Child doesn't read from this pipe
        {
            perror("close failed (pipe_fd[0])");
            exit(1);
        }

        if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) // Redirect child's output to the pipe
        {
            perror("dup2 failed (pipe_fd[1] to STDOUT)");
            exit(1);
        }

        if (close(pipe_fd[1]) < 0)
        {
            perror("close failed (pipe_fd[1])");
            exit(1);
        }
        if (file_input != -1 && file_input != -2 && dup2(file_input, STDIN_FILENO) == -1)
        {
            perror("dup2 failed for input");
            close(file_input);
            file_input = -2;
        }
        if (file_output != -1 && file_output != -2 && dup2(file_output, STDOUT_FILENO) == -1)
        {
            perror("dup2 failed for output");
            close(file_output);
            file_output = -1;
            // continue;
        }
        // Execute the command
        if (decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log) < 0)
        {
            fprintf(stderr, "Error: Command execution failed\n");
            exit(1);
        }

        exit(0);
    }

    if (*pgid == 0)
        *pgid = rc;

    fg_job *copy = malloc(sizeof(fg_job));
    if (!copy)
    {
        perror("malloc failed for fg_job");
        return -1;
    }
    copy->pid = rc;

    char cmd_name[LINE_MAX] = {0};
    for (int i = 0; i < (int)to_be_passed->size; i++)
    {
        char *cmd_part = strdup(((string_t *)to_be_passed->data)[i].data);
        if (!cmd_part)
        {
            perror("strdup failed for cmd_part");
            free(copy);
            return -1;
        }
        strcat(cmd_name, cmd_part);
        char *l = strdup(" ");
        if (!l)
        {
            perror("strdup failed for space");
            free(copy);
            return -1;
        }
        strcat(cmd_name, l);
        free(cmd_part);
        free(l);
    }
    strcat(cmd_name, " | ");

    copy->command_name = malloc(LINE_MAX * sizeof(char));
    if (!copy->command_name)
    {
        perror("malloc failed for command_name");
        free(copy);
        return -1;
    }
    strcpy(copy->command_name, cmd_name);
    *job_info = copy;
    vector_push_back(pids, copy);
    if (close(pipe_fd[1]) < 0) // Parent doesn't write to this pipe
    {
        perror("close failed (pipe_fd[1])");
        return -1;
    }

    // if (dup2(pipe_fd[0], STDIN_FILENO) < 0) // Redirect parent's input from the pipe
    // {
    //     perror("dup2 failed (pipe_fd[0] to STDIN)");
    //     return -1;
    // }

    // if (close(pipe_fd[0]) < 0)
    // {
    //     perror("close failed (pipe_fd[0])");
    //     return -1;
    // }
    vector_clear(to_be_passed);
    return 0;
}
