#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <dirent.h>
#include "stringlib.h"
#include "vectorlib.h"
#include "queue.h"

typedef struct
{
    int pid;            // process ID
    char *command_name; // name of the command (for printing messages)
    char *state;
    int job_number;     // job number [1], [2], etc
} bg_job; // for storing background jobs that are being executed



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
void log_function(vector_t *token_list, char *inp, char *prev_dir, char *home_dir, Queue *log_list, vector_t* bg_job_list);
void log_add(char *inp, Queue *log_list);
int execute_cmd(char *inp, char *home_dir, char *prev_dir, Queue *log_list, vector_t* bg_job_list, bool should_log);
int decide_and_call(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list,vector_t *bg_job_list, bool should_log);
int pipe_function(char *inp, vector_t *to_be_passed, char *home_dir, char *prev_dir, Queue *log_list, int* pipe_fd, vector_t* pids, vector_t* bg_job_list, bool should_log);
int activity_function(vector_t* bg_job_list);
#endif