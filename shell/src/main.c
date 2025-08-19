#include "../include/headerfiles.h"
#include "../include/shell.h"
#include "../include/queue.h"
#include <signal.h>
pid_t foreground_pgid = -1;
fg_job *current_fg_job = NULL;
int bg_job_no = 1; // to store the next job number index
// LLM
void sigint_handler(int signum)
{
    if (foreground_pgid > 0)
    {
        kill(-foreground_pgid, SIGINT);
    }
}
// LLM

void sigtstp_handler(int signum)
{
    if (current_fg_job == NULL)
    {
        return; // No foreground job to stop
    }

    // Send SIGTSTP to the entire process group
    kill(-(current_fg_job->pid), SIGTSTP);

    bg_job *current_job = malloc(sizeof(bg_job));
    current_job->command_name = strdup(current_fg_job->command_name);
    current_job->job_number = bg_job_no;
    current_job->pid = current_fg_job->pid;
    current_job->state = strdup("Stopped");

    // Print the message
    printf("\n[%d] Stopped   %s\n", bg_job_no, current_fg_job->command_name);

    free(current_fg_job->command_name);
    free(current_fg_job);
    current_fg_job = NULL;
}
int main()
{
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    char home_dir[] = "/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell";
    Queue *log_list = queue_create();
    char *prev_dir = malloc(4097 * sizeof(char));
    strcpy(prev_dir, home_dir);
    vector_t *bg_job_list = malloc(sizeof(vector_t));
    vector_init(bg_job_list, sizeof(bg_job), 0);
    while (1)
    {
        display_prompt(home_dir);
        // int status;

        char *inp = malloc(1025 * sizeof(char));
        if (get_command(inp))
        {
            printf("logout\n");
            for (int i = 0; i < (int)bg_job_list->size; i++)
            {
                pid_t child_pid = ((bg_job *)bg_job_list->data)[i].pid;
                kill(child_pid, SIGKILL);
            }
            free(inp);
            exit(0);
        }
        for (int i = 0; i < (int)bg_job_list->size; i++)
        {
            // LLM used
            int status;
            int result = (int)waitpid((((bg_job *)bg_job_list->data)[i].pid), &status, WUNTRACED | WNOHANG); // check status of bg process
            if (result == 0)
            {
                // still running
                ((bg_job *)bg_job_list->data)[i].state = strdup("Running");
            }
            else if (result == -1)
            {
                // error (maybe no such child anymore)
                perror("background process error");
            }
            else if (WIFSTOPPED(status))
            {
                // The child process is currently stopped.
                ((bg_job *)bg_job_list->data)[i].state = strdup("Stopped");
            }
            else
            {
                int exit_code = WEXITSTATUS(status);
                if (!exit_code)
                    printf("%s with pid %d exited normally\n", ((bg_job *)bg_job_list->data)[i].command_name, ((bg_job *)bg_job_list->data)[i].pid);
                else
                    printf("%s with pid %d exited abnormally\n", ((bg_job *)bg_job_list->data)[i].command_name, ((bg_job *)bg_job_list->data)[i].pid);
                vector_erase(bg_job_list, i, &((bg_job *)bg_job_list->data)[i]);
                i--;
            }
            // LLM used
        }
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