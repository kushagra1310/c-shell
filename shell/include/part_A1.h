#ifndef PART_A1_H // remember to see properly how macros work
#define PART_A1_H // remember to see why header files are important

// Function to display the shell prompt
void display_prompt(void);

// Functions used by show_prompt
char *get_username(int output_copy);
char *get_systemname(int output_copy);
char *get_current_dir(int output_copy);
void display_tilde(char *original_path);

#endif
