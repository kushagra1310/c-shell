// #define VECTOR_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/shell.h"
#include "../include/headerfiles.h"
#include <dirent.h>
#include <strings.h>

int string_comparator(const void *a, const void *b)
{
    const string_t *sa = (const string_t *)a;
    const string_t *sb = (const string_t *)b;
    return strcmp(sa->data, sb->data);
}
void print_lexicographically(DIR *d)
{
    struct dirent *file_dir;
    vector_t *file_list = malloc(sizeof(vector_t));
    vector_init(file_list, sizeof(string_t), 0);
    while ((file_dir = readdir(d)) != NULL)
    {
        if (file_dir->d_name[0] == '.')
            continue;
        string_t copy;
        string_init(&copy, file_dir->d_name);
        vector_push_back(file_list, &copy);
    }
    vector_sort(file_list, string_comparator);
    for (int i = 0; i < (int)file_list->size; i++)
    {
        printf("%s ", ((string_t *)file_list->data)[i].data);
    }
    for (int i = 0; i < (int)file_list->size; i++)
    {
        string_free(&(((string_t *)file_list->data)[i]));
    }
    printf("\n");
    vector_free(file_list);
    free(file_list);
}
void print_lexicographically_l(DIR *d)
{
    struct dirent *file_dir;
    vector_t *file_list = malloc(sizeof(vector_t));
    vector_init(file_list, sizeof(string_t), 0);
    while ((file_dir = readdir(d)) != NULL)
    {
        if (file_dir->d_name[0] == '.')
            continue;
        string_t copy;
        string_init(&copy, file_dir->d_name);
        vector_push_back(file_list, &copy);
    }
    vector_sort(file_list, string_comparator);
    for (int i = 0; i < (int)file_list->size; i++)
    {
        printf("%s\n", ((string_t *)file_list->data)[i].data);
    }
    for (int i = 0; i < (int)file_list->size; i++)
    {
        string_free(&(((string_t *)file_list->data)[i]));
    }
    vector_free(file_list);
    free(file_list);
}
void print_lexicographically_a(DIR *d)
{
    struct dirent *file_dir;
    vector_t *file_list = malloc(sizeof(vector_t));
    vector_init(file_list, sizeof(string_t), 0);
    while ((file_dir = readdir(d)) != NULL)
    {
        string_t copy;
        string_init(&copy, file_dir->d_name);
        vector_push_back(file_list, &copy);
    }
    vector_sort(file_list, string_comparator);
    for (int i = 0; i < (int)file_list->size; i++)
    {
        printf("%s ", ((string_t *)file_list->data)[i].data);
    }
    for (int i = 0; i < (int)file_list->size; i++)
    {
        string_free(&(((string_t *)file_list->data)[i]));
    }
    printf("\n");
    vector_free(file_list);
    free(file_list);
}
void print_lexicographically_al(DIR *d)
{
    struct dirent *file_dir;
    vector_t *file_list = malloc(sizeof(vector_t));
    vector_init(file_list, sizeof(string_t), 0);
    while ((file_dir = readdir(d)) != NULL)
    {
        string_t copy;
        string_init(&copy, file_dir->d_name);
        vector_push_back(file_list, &copy);
    }
    vector_sort(file_list, string_comparator);
    for (int i = 0; i < (int)file_list->size; i++)
    {
        printf("%s\n", ((string_t *)file_list->data)[i].data);
    }
    for (int i = 0; i < (int)file_list->size; i++)
    {
        string_free(&(((string_t *)file_list->data)[i]));
    }
    vector_free(file_list);
    free(file_list);
}
void check_and_print(DIR *d, int l_flag, int a_flag)
{
    if (l_flag && a_flag)
        print_lexicographically_al(d);
    else if (l_flag)
        print_lexicographically_l(d);
    else if (a_flag)
        print_lexicographically_a(d);
    else
        print_lexicographically(d);
}
void reveal_function(vector_t *token_list, char *home_dir, char *prev_dir)
{
    // printf("I'm here\n");
    int x_pointer = 1;
    char cur_dir[4097];
    getcwd(cur_dir, sizeof(cur_dir));
    int l_flag = 0, a_flag = 0;
    DIR *d;
    if (x_pointer == (int)token_list->size)
    {
        d = opendir(cur_dir);
        if (d == NULL)
        {
            // perror("opendir failed");
            printf("No such directory!\n");
            return;
        }
        check_and_print(d, l_flag, a_flag);
        return;
    }
    while (x_pointer < (int)token_list->size)
    {
        string_t temp = ((string_t *)token_list->data)[x_pointer];
        if (!strcmp(temp.data, "-"))
        {
            d = opendir(prev_dir);
            if (d == NULL)
            {
                // perror("opendir failed");
                printf("No such directory!\n");
                return;
            }
            check_and_print(d, l_flag, a_flag);
            break;
        }
        else if (!strcmp(temp.data, "."))
        {
            d = opendir(cur_dir);
            if (d == NULL)
            {
                // perror("opendir failed");
                printf("No such directory!\n");
                return;
            }
            check_and_print(d, l_flag, a_flag);
            break;
        }
        else if (!strcmp(temp.data, "~"))
        {
            d = opendir(home_dir);
            if (d == NULL)
            {
                // perror("opendir failed");
                printf("No such directory!\n");
                return;
            }
            check_and_print(d, l_flag, a_flag);
            break;
        }
        else if (!strcmp(temp.data, ".."))
        {

            char parent[4097];
            strcpy(parent, cur_dir);
            char *last_slash = strrchr(parent, '/');

            if (last_slash != NULL && last_slash != parent)
            {
                *last_slash = '\0'; // terminate before last directory
            }
            d = opendir(parent);
            if (d == NULL)
            {
                // perror("opendir failed");
                printf("No such directory!\n");
                return;
            }
            check_and_print(d, l_flag, a_flag);
            break;
        }
        else if (temp.data[0] == '-')
        {
            if (strchr(temp.data, 'l') && strchr(temp.data, 'a'))
            {
                l_flag = 1;
                a_flag = 1;
            }
            else if (strchr(temp.data, 'l'))
            {
                // x_pointer++;
                l_flag = 1;
            }
            else if (strchr(temp.data, 'a'))
            {
                // x_pointer++;
                a_flag = 1;
            }
            x_pointer++;
            if (x_pointer == (int)token_list->size)
            {
                d = opendir(home_dir);
                if (d == NULL)
                {
                    printf("No such directory!\n");
                    return;
                }
                check_and_print(d, l_flag, a_flag);
            }
        }
        else if (x_pointer == (int)token_list->size - 1)
        {
            d = opendir(temp.data);
            if (d == NULL)
            {
                // perror("opendir failed");
                printf("No such directory!\n");
                return;
            }
            check_and_print(d, l_flag, a_flag);
            break;
        }
        else
        {
            printf("Incorrect syntax for reveal!\n");
            break;
        }
    }
}
