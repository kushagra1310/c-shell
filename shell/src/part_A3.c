#define VECTOR_IMPLEMENTATION
#define DYNSTRING_IMPLEMENTATION
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/shell.h"
#include "../include/headerfiles.h"

vector_t *tokenize_input(char *inp)
{
    vector_t *token_list = malloc(sizeof(vector_t));
    vector_init_with_destructor(token_list, sizeof(string_t), 0, (vector_destructor_fn)string_free);

    string_t temp;
    string_init(&temp, "");
    for (int i = 0; i < (int)strlen(inp); i++)
    {
        if (inp[i] != '|' && inp[i] != '&' && inp[i] != '<' && inp[i] != '>' && inp[i] != ';' && inp[i] != ' ')
            string_append_char(&temp, inp[i]);
        else
        {
            if (temp.length > 0)
            {
                string_t copy;
                string_init(&copy, temp.data);
                vector_push_back(token_list, &copy);

                string_clear(&temp);
            }
            if (inp[i] == ' ' || inp[i] == '\t' || inp[i] == '\n' || inp[i] == '\r')
                continue;
            string_t delimit;
            string_init(&delimit, "");
            string_append_char(&delimit, inp[i]);
            if (inp[i] == '&' || inp[i] == '>')
            {
                if (i + 1 < (int)strlen(inp) && inp[i + 1] == inp[i])
                    string_append_char(&delimit, inp[++i]);
            }
            vector_push_back(token_list, &delimit);
        }
    }
    if (temp.length > 0)
    {
        string_t copy;
        string_init(&copy, temp.data);
        vector_push_back(token_list, &copy);
        string_clear(&temp);
    }
    else
        string_free(&temp);
    return token_list;
}

bool is_name(string_t token)
{
    if (strcmp(token.data, "|") == 0 || strcmp(token.data, "&") == 0 || strcmp(token.data, "&&") == 0 || strcmp(token.data, "<") == 0 || strcmp(token.data, ">") == 0 || strcmp(token.data, ">>") == 0 || strcmp(token.data, ";") == 0)
        return false;
    return true;
}

bool is_input(string_t token1, string_t token2)
{
    if (strcmp(token1.data, "<") != 0)
        return false;
    if (is_name(token2))
        return true;
    return false;
}

bool is_output(string_t token1, string_t token2)
{
    if (strcmp(token1.data, ">") != 0)
        return false;
    if (is_name(token2))
        return true;
    return false;
}

bool parse_atomic(vector_t *token_list, char *inp, int *x_pointer)
{
    if ((int)(token_list->size) - 1 - *x_pointer < 0)
        return false;
    string_t temp = ((string_t *)token_list->data)[*x_pointer];
    if (!is_name(temp))
        return false;
    else
        (*x_pointer)++;
    while (*x_pointer < (int)(token_list->size))
    {
        temp = ((string_t *)token_list->data)[*x_pointer];
        if (is_name(temp))
        {
            (*x_pointer)++;
        }
        else if (*x_pointer + 1 < (int)(token_list->size))
        {
            string_t temp_2 = ((string_t *)(token_list->data))[*x_pointer + 1];
            if (is_input(temp, temp_2) || is_output(temp, temp_2))
            {
                (*x_pointer)++;
            }
            else
                break;
        }
        else
            break;
    }
    return true;
}

bool parse_cmd_group(vector_t *token_list, char *inp, int *x_pointer)
{
    if ((int)(token_list->size) - 1 - *x_pointer < 0)
        return false;
    string_t temp = ((string_t *)token_list->data)[*x_pointer];
    if (!parse_atomic(token_list, inp, x_pointer))
    {
        return false;
    }
    for (int i = *x_pointer; i < (int)(token_list->size) && *x_pointer < (int)(token_list->size); i++)
    {
        temp = ((string_t *)token_list->data)[*x_pointer];
        if (strcmp(temp.data, "|") != 0)
        {
            return false;
        }
        else
            (*x_pointer)++;
        if (!parse_atomic(token_list, inp, x_pointer))
            return false;
    }
    return true;
}

bool parse_shell_cmd(char *inp)
{
    vector_t *token_list = tokenize_input(inp);
    int x_pointer = 0;
    if ((int)(token_list->size) - 1 - x_pointer < 0)
        return false;
    string_t temp = ((string_t *)token_list->data)[x_pointer];
    if (!parse_cmd_group(token_list, inp, &x_pointer))
    {
        return false;
    }
    for (int i = x_pointer; i < (int)(token_list->size) && x_pointer < (int)(token_list->size) - 1; i++)
    {
        temp = ((string_t *)token_list->data)[x_pointer];
        if (strcmp(temp.data, ";") != 0 && strcmp(temp.data, "&") != 0 && strcmp(temp.data, "&&") != 0)
            return false;
        else
            (x_pointer)++;
        if (strcmp(temp.data, "&") == 0 && x_pointer == (int)(token_list->size))
            break;
        if (!parse_cmd_group(token_list, inp, &x_pointer))
            return false;
    }
    vector_free(token_list);
    free(token_list);
    return true;
}
