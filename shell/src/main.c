#include "../include/headerfiles.h"
#include "../include/shell.h"
#include "../include/queue.h"
#include <signal.h>
pid_t foreground_pgid = -1;
fg_job *current_fg_job = NULL;
int bg_job_no = 1; // to store the next job number index
vector_t *bg_job_list;
pid_t shell_pgid;
// LLM
void sigint_handler(int signum)
{
    if (foreground_pgid > 0)
    {
        kill(-foreground_pgid, SIGINT); // sending SIGINT to the entire foreground process group(job)
        printf("\n");
    }
}
// LLM

void sigtstp_handler(int signum)
{
    return;
}

int main()
{
    shell_pgid = getpgrp();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    bg_job_list = malloc(sizeof(vector_t));
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    char home_dir[4097];
    getcwd(home_dir, sizeof(home_dir));
    // "/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell"; hardcoded file path
    Queue *log_list = queue_create();
    char *prev_dir = malloc(4097 * sizeof(char));
    strcpy(prev_dir, home_dir);
    vector_init(bg_job_list, sizeof(bg_job), 0);
    while (1)
    {
        display_prompt(home_dir);
        // int status;
        // if(current_fg_job)
        // printf("fgjob : %s\n",current_fg_job->command_name);
        char *inp = malloc(1025 * sizeof(char));
        if (get_command(inp))
        {
            printf("logout\n");
            for (int i = 0; i < (int)bg_job_list->size; i++)
            {
                pid_t child_pid = ((bg_job *)bg_job_list->data)[i].pid;
                kill(-child_pid, SIGKILL);
            }
            free(inp);
            free(prev_dir);
            exit(0);
        }
        // printf("size: %d\n", (int)bg_job_list->size);
        for (int i = 0; i < (int)bg_job_list->size; i++)
        {
            // A temporary pointer to the current job to make code cleaner
            bg_job *job = &(((bg_job *)bg_job_list->data)[i]);
            int status;

            // Poll the process status without blocking the shell
            pid_t result = waitpid(job->pid, &status, WNOHANG | WUNTRACED);

            if (result == 0)
            {
                // Result 0 means the process is still running.
                // We do nothing and continue to the next background job.
                continue;
            }
            else if (result == -1)
            {
                // An error occurred. This can happen if the process is already gone.
                // It's safest to remove it from our list.
                perror("waitpid background error");
                vector_erase(bg_job_list, i, NULL);
                i--; // Decrement i because the list is now shorter.
            }
            else if (WIFEXITED(status))
            {
                // The process finished its task normally.
                fprintf(stderr, "\n%s with pid %d exited normally\n", job->command_name, job->pid);
                vector_erase(bg_job_list, i, NULL);
                i--; // Decrement i because the list is now shorter.
            }
            else if (WIFSIGNALED(status))
            {
                // The process was killed by a signal.
                fprintf(stderr, "\n%s with pid %d was terminated by a signal\n", job->command_name, job->pid);
                vector_erase(bg_job_list, i, NULL);
                i--; // Decrement i because the list is now shorter.
            }
            else if (WIFSTOPPED(status))
            {
                // The process was stopped (e.g., by a signal from 'ping').
                // We just update its state. We don't remove it.
                free(job->state);
                job->state = strdup("Stopped");
            }
        }
        // LLM used
        if (!parse_shell_cmd(inp))
            printf("Invalid Syntax!");
        else
        {
            execute_cmd(inp, home_dir, prev_dir, log_list, bg_job_list, true);
        }
        free(inp);
    }
    return 0;
}