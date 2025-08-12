#ifndef SHELL_H
#define SHELL_H

char *get_username(int output_copy);
char *get_systemname(int output_copy);
char *get_current_dir(int output_copy);
void display_tilde(char *original_path);
void display_prompt(void);
void get_command(char *buf);

#endif
