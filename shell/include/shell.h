#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <dirent.h>
#include "stringlib.h"
#include "vectorlib.h"
#include "queue.h"

typedef struct
{
    vector_t *args;    // arguments for a command
    char *input_file;  // input file if it exists
    char *output_file; // output file if it exists
} command_t;

void get_command(char *buf);

bool parse_shell_cmd(char *inp);

void display_prompt(char *home_dir);

char *get_username(int output_copy);
char *get_systemname(int output_copy);
char *get_current_dir(int output_copy);
void display_tilde(char *original_path, char *home);

vector_t *tokenize_input(char *inp);

bool is_name(string_t token);
bool is_input(string_t token1, string_t token2);
bool is_output(string_t token1, string_t token2);
bool parse_atomic(vector_t *token_list, char *inp, int *x_pointer);
bool parse_cmd_group(vector_t *token_list, char *inp, int *x_pointer);

void hop_function(vector_t *token_list, char *home_dir, char *prev_dir);
void reveal_function(vector_t *token_list, char *home_dir, char *prev_dir);
int string_comparator(const void *a, const void *b);
void print_lexicographically(DIR *d);
void log_function(vector_t *token_list, char *inp, char *prev_dir, char *home_dir, Queue *log_list);
void log_add(char *inp, Queue *log_list);
int execute_cmd(char *inp, char *home_dir, char *prev_dir, Queue *log_list);
#endif