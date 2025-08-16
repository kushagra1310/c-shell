#include "../include/headerfiles.h"
#include "../include/shell.h"
#include "../include/queue.h"
int main()
{
    char home_dir[] = "/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell";
    Queue *log_list = queue_create();
    char *prev_dir = malloc(4097 * sizeof(char));
    strcpy(prev_dir, home_dir);
    FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
    if (log_file)
    {
        char buffer[4097];
        while (fgets(buffer, sizeof(buffer), log_file) != NULL)
        {
            int len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
            {
                buffer[len - 1] = '\0';
            }
            char *line = strdup(buffer);
            if (!line)
            {
                perror("Failed to duplicate string");
                break;
            }
            enqueue(log_list, line);
        }
        fclose(log_file);
    }
    // pushing to the queue previous log_file contents so that record of previous session is preserved for this one
    while (1)
    {
        display_prompt(home_dir);
        char *inp = malloc(1025 * sizeof(char));
        get_command(inp);
        if (!parse_shell_cmd(inp))
            printf("Invalid Syntax!");
        else
        {
            execute_cmd(inp, home_dir, prev_dir, log_list);
        }
        free(inp);
    }
    return 0;
}