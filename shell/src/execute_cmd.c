#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/headerfiles.h"
#include "../include/shell.h"

extern pid_t foreground_pgid;
extern int bg_job_no;
extern fg_job *current_fg_job;

int decide_and_call(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, vector_t *bg_job_list, bool should_log)
{
    if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "hop") == 0)
    {
        hop_function(to_be_passed, home_dir, prev_dir);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "reveal") == 0)
    {
        reveal_function(to_be_passed, home_dir, prev_dir);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "activities") == 0)
    {
        activity_function(bg_job_list);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "ping") == 0)
    {
        char *pid_str = strdup(((string_t *)to_be_passed->data)[1].data);
        char *endptr;
        long pid_num = strtol(pid_str, &endptr, 10);
        if (*endptr != '\0')
        {
            return 1;
        }
        char *sig_str = strdup(((string_t *)to_be_passed->data)[2].data);

        long sig_num = strtol(sig_str, &endptr, 10);
        if (*endptr != '\0')
        {
            return 1;
        }
        ping_function((int)pid_num, (int)sig_num);
        free(sig_str);
        free(pid_str);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "fg") == 0)
    {
        int job_to_fg = -1;
        if (to_be_passed->size < 2)
        {
            // No job number provided, use the most recent one
            if (bg_job_list->size > 0)
            {
                job_to_fg = bg_job_list->size - 1;
            }
        }
        else
        {
            char *job_to_fg_str = strdup(((string_t *)to_be_passed->data)[1].data);
            job_to_fg = strtol(job_to_fg_str, NULL, 10);
        }
        fg(bg_job_list, job_to_fg);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "bg") == 0)
    {
        int job_to_bg = -1;
        if (to_be_passed->size < 2)
        {
            if (bg_job_list->size > 0)
            {
                job_to_bg = bg_job_list->size - 1;
            }
        }
        else
        {
            char *job_to_bg_str = strdup(((string_t *)to_be_passed->data)[1].data);
            job_to_bg = strtol(job_to_bg_str, NULL, 10);
        }
        bg(bg_job_list, job_to_bg);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "log") == 0)
    {
        log_function(to_be_passed, inp, prev_dir, home_dir, log_list, bg_job_list);
    }
    else if ((int)to_be_passed->size > 0 && strcmp(((string_t *)to_be_passed->data)[0].data, "log") != 0)
    {
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
        execvp(args[0], args);
        perror("exec failed");
        return 1;
    }
    return 0;
}
int execute_cmd(char *inp, char *home_dir, char *prev_dir, Queue *log_list, vector_t *bg_job_list, bool should_log)
{
    vector_t *token_list = tokenize_input(inp);
    vector_t *pids = malloc(sizeof(vector_t));
    // vector_init_with_destructor(pids, sizeof(fg_job), 0, (vector_destructor_fn)fg_job_destructor);
    for (int i = 0; i < (int)token_list->size; i++)
    {
        if (!strcmp(((string_t *)token_list->data)[i].data, "log"))
        {
            should_log = false;
            break;
        }
    }
    if (should_log)
        log_add(inp, log_list);

    vector_init(pids, sizeof(fg_job), 0);
    vector_t *to_be_passed = malloc(sizeof(vector_t));
    vector_init_with_destructor(to_be_passed, sizeof(string_t), 0, (vector_destructor_fn)string_free);
    int x_pointer = 0;
    int terminal_output_copy = dup(STDOUT_FILENO);
    int terminal_input_copy = dup(STDIN_FILENO);
    int file_input = -1, file_output = -1;
    pid_t current_pgid = 0; // local pgid
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
        else if (!strcmp(temp.data, ";"))
        {
            if (strcmp(((string_t *)to_be_passed->data)[0].data, "hop") == 0)
            {
                // If it is hop, run directly as ow it wouldn't be visible in the prompt
                decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
            }
            else
            {
                int rc = fork();
                if (!rc)
                {
                    decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
                    exit(0);
                }
                waitpid(rc, NULL, 0);
            }
            vector_clear(to_be_passed);
        }
        else if (!strcmp(temp.data, "&"))
        {
            // int new_job_no = (bg_job_list->size) ? (((bg_job *)bg_job_list->data)[bg_job_list->size].job_number + 1) : 1;
            int new_job_no = (bg_job_list->size) ? (((bg_job *)bg_job_list->data)[bg_job_list->size - 1].job_number + 1) : 1;
            bg_job_no = new_job_no + 1;
            char cmd_name[4097] = {0};
            for (int i = 0; i < (int)to_be_passed->size; i++)
            {
                char *cmd_part = strdup(((string_t *)to_be_passed->data)[i].data);
                strcat(cmd_name, cmd_part);
                char *l = strdup(" ");
                strcat(cmd_name, l);
            }
            int rc = fork();
            if (!rc)
            {
                // LLM used
                int dev_null_fd = open("/dev/null", O_RDONLY);
                if (dev_null_fd != -1)
                {
                    dup2(dev_null_fd, STDIN_FILENO); // point stdin to /dev/null to avoid terminal access for input
                    close(dev_null_fd);
                }
                // LLM used
                if (!decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log))
                {
                    exit(0);
                }
                else
                {
                    exit(1);
                }
            }
            bg_job *current_job = malloc(sizeof(bg_job));
            current_job->command_name = strdup(cmd_name);
            current_job->job_number = new_job_no;
            current_job->pid = rc;
            current_job->state = strdup("Running");
            vector_push_back(bg_job_list, current_job);
            printf("[%d] %d\n", new_job_no, rc);
            vector_clear(to_be_passed);
        }
        else if (!strcmp(temp.data, "|"))
        {
            int pipe_fd[2];
            pipe(pipe_fd);
            pipe_function(inp, to_be_passed, home_dir, prev_dir, log_list, pipe_fd, pids, bg_job_list, should_log, &current_pgid);
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
        if (strcmp(((string_t *)to_be_passed->data)[0].data, "hop") == 0 || strcmp(((string_t *)to_be_passed->data)[0].data, "log") == 0)
        {
            // If it is hop, run directly as ow it wouldn't be visible in the prompt
            decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
        }
        else if (strcmp(((string_t *)to_be_passed->data)[0].data, "fg") == 0 || strcmp(((string_t *)to_be_passed->data)[0].data, "bg") == 0)
        {
            decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
        }
        else
        {
            int rc = fork();
            if (!rc)
            {
                setpgid(0, current_pgid);
                decide_and_call(inp, to_be_passed, home_dir, prev_dir, log_list, bg_job_list, should_log);
                exit(0);
            }
            if (current_pgid == 0)
            {
                current_pgid = rc;
            }
            fg_job *copy = malloc(sizeof(fg_job));
            copy->pid = rc;
            char cmd_name[4097] = {0};
            for (int i = 0; i < (int)to_be_passed->size; i++)
            {
                char *cmd_part = strdup(((string_t *)to_be_passed->data)[i].data);
                strcat(cmd_name, cmd_part);
                char *l = strdup(" ");
                strcat(cmd_name, l);
            }
            copy->command_name = strdup(cmd_name);
            if (!current_fg_job)
            {
                current_fg_job = copy;
            }
            else
            {
                strcat(current_fg_job->command_name, copy->command_name);
            }
            vector_push_back(pids, copy);
        }
    }
    // Restoring standard input output from terminal just to be sure
    dup2(terminal_input_copy, STDIN_FILENO);
    dup2(terminal_output_copy, STDOUT_FILENO);
    close(terminal_input_copy);
    close(terminal_output_copy);
    if (pids->size > 0)
    {
        foreground_pgid = current_pgid;
    }

    for (int i = 0; i < (int)pids->size; i++)
    {
        pid_t current_pid_ptr = ((fg_job *)pids->data)[i].pid;
        waitpid(current_pid_ptr, NULL, 0);
    }
    foreground_pgid = -1;

    vector_free(pids);
    free(pids);
    vector_free(to_be_passed);
    free(to_be_passed);
    vector_free(token_list);
    free(token_list);
    return 0;
}
