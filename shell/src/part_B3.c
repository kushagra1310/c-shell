#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
void log_add(char *inp, Queue *log_list)
{
    FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "a");
    if ((queue_get_size(log_list) > 0 && strcmp(inp, queue_get_at(log_list, (int)queue_get_size(log_list) - 1)) != 0) || !queue_get_size(log_list))
    {
        char *inp_dup = strdup(inp);
        enqueue(log_list, inp_dup);
        if (queue_get_size(log_list) >= 15)
        {
            dequeue(log_list);
            fclose(log_file);
            log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "w");
            Node *pointer = log_list->rear;
            while (pointer != NULL)
            {
                fwrite(pointer->data, sizeof(char), strlen(pointer->data), log_file);
                fwrite("\n", sizeof(char), 1, log_file);
                pointer = pointer->next;
            }
            fclose(log_file);
        }
        else
        {
            fwrite(inp, sizeof(char), strlen(inp) + 1, log_file);
            fwrite("\n", sizeof(char), 1, log_file);
        }
    }

    fclose(log_file);
}
void log_function(vector_t *token_list, char *inp, char *prev_dir, char *home_dir, Queue *log_list)
{
    string_t temp;
    if (token_list->size > 0 && token_list->size <= 3)
        temp = ((string_t *)token_list->data)[0];
    else
    {
        printf("Empty command\n");
        return;
    }
    if (!strcmp(temp.data, "log"))
    {
        if (token_list->size > 1)
        {
            temp = ((string_t *)token_list->data)[1];
            if (!strcmp(temp.data, "purge"))
            {
                FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "w");
                fclose(log_file);
            }
            else if (!strcmp(temp.data, "execute"))
            {
                if (token_list->size > 2)
                {
                    temp = ((string_t *)token_list->data)[2];
                    char *end;
                    long index = strtol(temp.data, &end, 10);
                    if (*end != '\0' || index >= 15)
                    {
                        printf("Invalid log request\n");
                        return;
                    }
                    else
                    {
                        FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
                        int count = 0;
                        char ch;
                        while ((ch = getc(log_file)) != EOF && count < index)
                        {
                            if (ch == '\n')
                                count++;
                        }
                        if (ch == EOF)
                        {
                            printf("Invalid log request\n");
                            return;
                        }
                        fseek(log_file, -1, SEEK_CUR);
                        char str[4097];
                        fgets(str, 4097, log_file);
                        execute_cmd(str, prev_dir, home_dir, log_list);
                        fclose(log_file);
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
            FILE *log_file = fopen("/home/kushagra-agrawal/Desktop/osn/mini-project-1-kushagra1310/shell/src/log_file.txt", "r");
            if (log_file)
            {
                char buffer[4097];
                while (fgets(buffer, sizeof(buffer), log_file) != NULL)
                {
                    printf("%s\n",buffer);
                }
                fclose(log_file);
            }
        }
    }
    else
    {
        log_add(inp, log_list);
    }
}
