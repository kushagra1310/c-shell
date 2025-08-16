#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/headerfiles.h"
#include "../include/shell.h"
int decide_and_call(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list)
{
    if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "hop") == 0)
    {
        hop_function(to_be_passed, home_dir, prev_dir);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "reveal") == 0)
    {
        reveal_function(to_be_passed, home_dir, prev_dir);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "log") != 0)
    {
        int rc = fork();
        char **args = malloc(4096 * sizeof(char *));
        if ((int)to_be_passed->size > 4096)
        {
            printf("Argument limit reached\n");
        }
        int args_count = 0;
        for (args_count = 0; args_count < (int)to_be_passed->size; args_count++)
        {
            string_t temp = ((string_t *)to_be_passed->data)[args_count];
            args[args_count] = strdup(temp.data);
        }
        args[args_count] = NULL;
        if (!rc)
        {
            execvp(args[0], args);
            perror("exec failed");
            return 1;
        }
        else if (rc > 0)
            wait(NULL);
        else
        {
            perror("fork failed");
            return 1;
        }
        for (int i = 0; i < args_count; i++)
        {
            free(args[i]);
        }
        free(args);
    }
    log_function(to_be_passed, inp, prev_dir, home_dir, log_list);
    return 0;
}
int execute_cmd(char *inp, char *home_dir, char *prev_dir, Queue *log_list)
{
    vector_t *token_list = tokenize_input(inp);
    vector_t *pids = malloc(sizeof(vector_t));
    vector_init(pids, sizeof(pid_t), 0);
    vector_t *to_be_passed = malloc(sizeof(vector_t));
    vector_init_with_destructor(to_be_passed, sizeof(string_t), 0, (vector_destructor_fn)string_free);
    int x_pointer = 0;
    int terminal_output_copy = dup(STDOUT_FILENO);
    int terminal_input_copy = dup(STDIN_FILENO);
    int file_input = -1, file_output = -1;
    while (x_pointer < (int)token_list->size)
    {
        string_t temp = ((string_t *)token_list->data)[x_pointer++];
        if (!strcmp(temp.data, "<"))
        {
            if (x_pointer < (int)token_list->size)
            {
                temp = ((string_t *)token_list->data)[x_pointer++];
                if (file_input != -1)
                    close(file_input);
                file_input = open(temp.data, O_RDONLY);
                dup2(file_input, STDIN_FILENO);
            }
        }
        else if (!strcmp(temp.data, ">"))
        {
            if (x_pointer < (int)token_list->size)
            {
                temp = ((string_t *)token_list->data)[x_pointer++];
                if (file_output != -1)
                    close(file_output);
                file_output = open(temp.data, O_WRONLY | O_CREAT, 0666);
                dup2(file_output, STDOUT_FILENO);
            }
        }
        else if (!strcmp(temp.data, ">>"))
        {
            if (x_pointer < (int)token_list->size)
            {
                temp = ((string_t *)token_list->data)[x_pointer++];
                file_output = open(temp.data, O_WRONLY | O_APPEND | O_CREAT, 0666);
                dup2(file_output, STDOUT_FILENO);
            }
        }
        else if (!strcmp(temp.data, "&") || !strcmp(temp.data, ";") || !strcmp(temp.data, "&&"))
        {
            decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list);
            vector_clear(to_be_passed);
            dup2(terminal_input_copy, STDIN_FILENO);
            dup2(terminal_output_copy, STDOUT_FILENO);
            break;
        }
        else if (!strcmp(temp.data, "|"))
        {
            int pipe_fd[2];
            pipe(pipe_fd);
            pipe_function(inp, to_be_passed, home_dir, prev_dir, log_list, pipe_fd, pids);
        }
        else
        {
            string_t copy;
            string_init(&copy, temp.data);
            vector_push_back(to_be_passed, &copy);
            string_clear(&temp);
        }
    }
    if (to_be_passed->size > 0)
    {
        if (strcmp(((string_t *)to_be_passed->data)[0].data, "hop") == 0)
        {
            // If it is hop, run directly as ow it wouldn't be visible in the prompt
            decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list);
        }
        else
        {
            int rc = fork();
            if (!rc)
            {
                decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list);
                exit(0);
            }
            vector_push_back(pids, &rc);
        }
    }
    // Restoring standard input output from terminal just to be sure
    dup2(terminal_input_copy, STDIN_FILENO);
    dup2(terminal_output_copy, STDOUT_FILENO);
    close(terminal_input_copy);
    close(terminal_output_copy);
    // Waiting for completion
    for (int i = 0; i < (int)pids->size; i++)
    {
        pid_t current_pid = ((pid_t *)pids->data)[i];
        waitpid(current_pid, NULL, 0);
    }
    vector_free(pids);
    free(pids);
    vector_free(to_be_passed);
    free(to_be_passed);
    vector_free(token_list);
    free(token_list);
    return 0;
}
