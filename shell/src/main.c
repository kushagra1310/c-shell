#include "../include/headerfiles.h"
#include "../include/shell.h"
#include "../include/queue.h"
#include <signal.h>
pid_t foreground_pgid = -1;
fg_job *current_fg_job = NULL;
int bg_job_no = 1; // to store the next job number index
vector_t *bg_job_list;
pid_t shell_pgid;
int terminal_output_copy;
int terminal_input_copy;
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
    terminal_output_copy = dup(STDOUT_FILENO);
    terminal_input_copy = dup(STDIN_FILENO);
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
        for (int i = 0; i < (int)bg_job_list->size; i++)
        {
            bg_job *job = &(((bg_job *)bg_job_list->data)[i]);
            int status;

            // Poll the process status without blocking the shell
            pid_t result = waitpid(job->pid, &status, WNOHANG | WUNTRACED);

            if (result == 0)
            {
                // process is running
                continue;
            }
            else if (result == -1)
            {
                // error occoured so remove from list
                perror("waitpid background error");
                vector_erase(bg_job_list, i, NULL);
                i--; // because vector is shifted by one to the left
            }
            else if (WIFEXITED(status))
            {
                // finished its task normally.
                fprintf(stderr, "\n%s with pid %d exited normally\n", job->command_name, job->pid);
                vector_erase(bg_job_list, i, NULL);
                i--;
            }
            else if (WIFSIGNALED(status))
            {
                // The process was killed by a signal.
                fprintf(stderr, "\n%s with pid %d was terminated by a signal\n", job->command_name, job->pid);
                vector_erase(bg_job_list, i, NULL);
                i--;
            }
            else if (WIFSTOPPED(status))
            {
                // process stopped
                free(job->state);
                job->state = strdup("Stopped");
            }
        }
        // LLM used
        if (!parse_shell_cmd(inp))
            printf("Invalid Syntax!\n");
        else
        {
            execute_cmd(inp, home_dir, prev_dir, log_list, bg_job_list, true);
        }
        free(inp);
    }
    return 0;
}
