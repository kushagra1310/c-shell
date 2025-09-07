#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
extern char* log_file_name;
void log_add(char *inp, Queue *log_list)
{
    // FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
    FILE *log_file = fopen(log_file_name, "r");
    int line_count = 0;
    char last_line[4097] = "";
    if (log_file != NULL)
    {
        char buffer[4097];
        while (fgets(buffer, sizeof(buffer), log_file) != NULL)
        {
            line_count++;
            strcpy(last_line, buffer);
        }
        // remove trailing newline from last_line for accurate comparison
        last_line[strcspn(last_line, "\n")] = 0;
        fclose(log_file);
    }
    // if last line same don't add
    if (strcmp(inp, last_line) == 0)
    {
        return;
    }
    if (line_count < 15)
    {
        // appending
        // log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "a");
        log_file = fopen(log_file_name, "a");
        if (log_file == NULL)
        {
            perror("fopen failed");
            return;
        }
        fprintf(log_file, "%s\n", inp);
        fclose(log_file);
    }
    else
    {
        // history full
        // log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
        log_file = fopen(log_file_name, "r");
        if (log_file == NULL)
        {
            perror("fopen failed");
            return;
        }

        char *temp_lines[15];
        char buffer[4097];
        int i = 0;

        // Read all 15 lines into a temporary array in memory
        while (fgets(buffer, sizeof(buffer), log_file) != NULL)
        {
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline
            temp_lines[i++] = strdup(buffer);
        }
        fclose(log_file);
        // log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "w");
        log_file = fopen(log_file_name, "w");
        if (log_file == NULL)
        {
            perror("fopen failed");
            return;
        }

        // write back lines 2 through 15 (indices 1 to 14)
        for (i = 1; i < 15; i++)
        {
            fprintf(log_file, "%s\n", temp_lines[i]);
            free(temp_lines[i]);
        }
        free(temp_lines[0]);

        // most recent command
        fprintf(log_file, "%s\n", inp);
        fclose(log_file);
    }
}
void log_function(vector_t *token_list, char *inp, char *prev_dir, char *home_dir, Queue *log_list, vector_t *bg_job_list)
{
    string_t temp;
    if ((int)token_list->size > 0)
        temp = ((string_t *)token_list->data)[0];
    else
    {
        printf("Empty command\n");
        return;
    }
    if (!strcmp(temp.data, "log"))
    {
        if ((int)token_list->size > 3)
        {
            printf("Empty/Invalid command %d arguments\n", (int)token_list->size);
            return;
        }

        if ((int)token_list->size > 1)
        {
            temp = ((string_t *)token_list->data)[1];
            if (!strcmp(temp.data, "purge"))
            {
                // FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "w");
                FILE *log_file = fopen(log_file_name, "w");
                fclose(log_file);
                while (!queue_is_empty(log_list))
                    dequeue(log_list);
            }
            else if (!strcmp(temp.data, "execute"))
            {
                if (token_list->size > 2)
                {
                    temp = ((string_t *)token_list->data)[2];
                    char *end;
                    long index = strtol(temp.data, &end, 10);
                    if (*end != '\0' || index > 15 || index < 1)
                    {
                        printf("Invalid log request\n");
                        return;
                    }
                    else
                    {
                        // FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
                        FILE *log_file = fopen(log_file_name, "r");
                        int total_lines = 0;
                        char buff[LINE_MAX];
                        while (fgets(buff, sizeof(buff), log_file))
                        {
                            total_lines++;
                        }
                        rewind(log_file);
                        char command_to_execute[LINE_MAX];
                        for (int i = 0; i < total_lines-index+1; i++)
                        {
                            if (fgets(command_to_execute, sizeof(command_to_execute), log_file) == NULL)
                            {
                                printf("Error reading log file\n");
                                fclose(log_file);
                                return;
                            }
                        }
                        fclose(log_file);
                        command_to_execute[strcspn(command_to_execute, "\n")]='\0';
                        // printf("executing: %s\n",command_to_execute);
                        execute_cmd(command_to_execute, home_dir, prev_dir, log_list, bg_job_list, false);
                    }
                }
            }
            else
            {
                printf("Invalid log request\n");
                return;
            }
        }
        else
        {
            // FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
            FILE *log_file = fopen(log_file_name, "r");
            if (log_file)
            {
                char buffer[4097];
                while (fgets(buffer, sizeof(buffer), log_file) != NULL)
                {
                    printf("%s", buffer);
                }
                fclose(log_file);
            }
        }
    }
    // else
    // {
    //     log_add(inp, log_list);
    // }
}
