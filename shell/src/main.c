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
char* log_file_name;
// ############## LLM Generated Code Begins ##############
void sigint_handler(int signum)
{
    if (foreground_pgid > 0)
    {
        kill(-foreground_pgid, SIGINT); // sending SIGINT to the entire foreground process group(job)
        printf("\n");
    }
}
void get_log_file_path(char *log_path_buffer, size_t buffer_size)
{
    // 1. Get the user's home directory path
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL)
    {
        // Fallback if HOME is not set, though this is rare
        fprintf(stderr, "Warning: HOME environment variable not set.\n");
        strncpy(log_path_buffer, "log_file", buffer_size);
        return;
    }

    // 2. Safely construct the full path (e.g., /home/kushagra/myshell_log)
    snprintf(log_path_buffer, buffer_size, "%s/log_file", home_dir);
}
// ############## LLM Generated Code Ends ##############

void sigtstp_handler(int signum)
{
    return;
}

int main()
{
    terminal_output_copy = dup(STDOUT_FILENO);
    terminal_input_copy = dup(STDIN_FILENO);
    shell_pgid = getpgrp();
    signal(SIGTTIN, SIG_IGN); // ignoring the shortcut key signals so that they don't kill the shell
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    bg_job_list = malloc(sizeof(vector_t));
    signal(SIGINT, sigint_handler); // special handler for ctrl c
    signal(SIGTSTP, sigtstp_handler); // for ctrl z, just ignore for now so that the shell doesn't act on it
    char home_dir[4097];
    getcwd(home_dir, sizeof(home_dir));
    log_file_name=malloc(4097*sizeof(char));
    get_log_file_path(log_file_name, 4096);
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
        // ############## LLM Generated Code Starts ##############
        for (int i = 0; i < (int)bg_job_list->size; i++)
        {
            bg_job *job = &(((bg_job *)bg_job_list->data)[i]);
            int status;

            // wait for result without blocking shell
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
                // process was killed by a signal.
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
        // ############## LLM Generated Code Ends ##############
        if (!parse_shell_cmd(inp))
        {
            printf("Invalid Syntax!\n");
            log_add(inp, log_list);
        }
        else
        {
            execute_cmd(inp, home_dir, prev_dir, log_list, bg_job_list, true);
        }
        free(inp);
    }
    return 0;
}
