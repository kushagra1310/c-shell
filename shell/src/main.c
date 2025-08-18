#include "../include/headerfiles.h"
#include "../include/shell.h"
#include "../include/queue.h"
int main()
{
    char home_dir[] = "/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell";
    Queue *log_list = queue_create();
    char *prev_dir = malloc(4097 * sizeof(char));
    strcpy(prev_dir, home_dir);
    // FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
    vector_t *bg_job_list = malloc(sizeof(vector_t));
    vector_init(bg_job_list, sizeof(bg_job), 0);
    while (1)
    {
        display_prompt(home_dir);
        // int status;
        
        char *inp = malloc(1025 * sizeof(char));
        get_command(inp);
        for (int i = 0; i < (int)bg_job_list->size; i++)
        {
            // LLM used
            int status;
            int result = (int)waitpid((((bg_job *)bg_job_list->data)[i].pid), &status, WUNTRACED | WNOHANG); // check status of bg process
            if (result == 0)
            {
                // still running
                ((bg_job *)bg_job_list->data)[i].state=strdup("Running");
            }
            else if (result == -1)
            {
                // error (maybe no such child anymore)
                perror("background process error");
            }
            else if (WIFSTOPPED(status))
            {
                // The child process is currently stopped.
                ((bg_job *)bg_job_list->data)[i].state=strdup("Stopped");
            }
            else
            {
                int exit_code = WEXITSTATUS(status);
                if(!exit_code)
                printf("%s with pid %d exited normally\n",((bg_job *)bg_job_list->data)[i].command_name,((bg_job *)bg_job_list->data)[i].pid);
                else
                printf("%s with pid %d exited abnormally\n",((bg_job *)bg_job_list->data)[i].command_name,((bg_job *)bg_job_list->data)[i].pid);
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