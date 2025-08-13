#include "../include/headerfiles.h"
#include "../include/shell.h"
char *get_username(int output_copy)
{
    int fd[2]; // file descriptors for piping
    if (pipe(fd) == -1)
    {
        perror("pipe failed");
        exit(1);
    }
    int rc = fork();
    if (rc == -1)
    {
        perror("fork error");
        exit(1);
    }
    else if (!rc)
    {
        close(fd[0]);               // closing the read end of the pipe as child just writes
        dup2(fd[1], STDOUT_FILENO); // to get output of terminal onto the pipe
        execlp("whoami", "whoami", NULL);
        perror("exec failure");
        exit(1);
    }
    else
    {
        wait(NULL);
        close(fd[1]);
        dup2(output_copy, STDOUT_FILENO);
        char *store_username = malloc(1025 * sizeof(char));
        int bytes_read = read(fd[0], store_username, 1024);
        if (bytes_read < 0)
        {
            perror("read unsuccessful");
            free(store_username); // free memory before exiting
            exit(1);
        }
        char *newline = strchr(store_username, '\n');
        if (newline)
            *newline = '\0';

        store_username[bytes_read] = '\0';
        close(fd[0]);
        return store_username;
    }
}

char *get_systemname(int output_copy)
{
    int fd[2]; // file descriptors for piping
    if (pipe(fd) == -1)
    {
        perror("pipe failed");
        exit(1);
    }
    int rc = fork();
    if (rc == -1)
    {
        perror("fork error");
        exit(1);
    }
    else if (!rc)
    {
        close(fd[0]);               // closing the read end of the pipe as child just writes
        dup2(fd[1], STDOUT_FILENO); // to get output of terminal onto the pipe
        execlp("hostname", "hostname", NULL);
        perror("exec failure");
        exit(1);
    }
    else
    {
        wait(NULL);
        close(fd[1]);
        dup2(output_copy, STDOUT_FILENO);
        char *store_systemname = malloc(1025 * sizeof(char));
        int bytes_read = read(fd[0], store_systemname, 1024);
        if (bytes_read < 0)
        {
            perror("read unsuccessful");
            free(store_systemname); // free memory before exiting
            exit(1);
        }
        char *newline = strchr(store_systemname, '\n');
        if (newline)
            *newline = '\0';

        store_systemname[bytes_read] = '\0';
        close(fd[0]);
        return store_systemname;
    }
}

char *get_current_dir(int output_copy)
{
    int fd[2]; // file descriptors for piping
    if (pipe(fd) == -1)
    {
        perror("pipe failed");
        exit(1);
    }
    int rc = fork();
    if (rc == -1)
    {
        perror("fork error");
        exit(1);
    }
    else if (!rc)
    {
        close(fd[0]);               // closing the read end of the pipe as child just writes
        dup2(fd[1], STDOUT_FILENO); // to get output of terminal onto the pipe
        execlp("pwd", "pwd", NULL);
        perror("exec failure");
        exit(1);
    }
    else
    {
        wait(NULL);
        close(fd[1]);
        dup2(output_copy, STDOUT_FILENO);
        char *store_dir_name = malloc(1025 * sizeof(char));
        int bytes_read = read(fd[0], store_dir_name, 1024);
        if (bytes_read < 0)
        {
            perror("read unsuccessful");
            free(store_dir_name); // free memory before exiting
            exit(1);
        }
        char *newline = strchr(store_dir_name, '\n');
        if (newline)
            *newline = '\0';

        store_dir_name[bytes_read] = '\0';
        close(fd[0]);
        return store_dir_name;
    }
}

void display_tilde(char *original_path)
{
    char home[4097]; // check document again for clarification
    if(!getcwd(home, sizeof(home)))
    {
        perror("getcwd failed");
        exit(1);
    }
    int home_len = strlen(home);
    if (strncmp(original_path, home, home_len) == 0) // check if it starts with home directory
    {
        int new_len = strlen(original_path) - home_len + 1; // +1 for ~
        char *result = malloc(new_len + 1);                // +1 for \0
        if (!result)
        {
            perror("malloc failed");
            exit(1);
        }
        result[0] = '~';
        strcpy(result + 1, original_path + home_len); // replacing the relevant part of the string from the original string
        strcpy(original_path, result);
        free(result); // free memory after use
        return;
    }
    return; // no change
}

void display_prompt()
{
    int terminal_output_copy = dup(STDOUT_FILENO);
    char *username, *systemname, *current_dir;
    username = get_username(terminal_output_copy);
    systemname = get_systemname(terminal_output_copy);
    current_dir = get_current_dir(terminal_output_copy);
    display_tilde(current_dir);
    printf("<%s@%s:%s>", username, systemname, current_dir);

    // free allocated memory
    free(username);
    free(systemname);
    free(current_dir);
}
