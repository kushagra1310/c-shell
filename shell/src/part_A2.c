#include "../include/headerfiles.h"
#include "../include/shell.h"
int get_command(char *buf)
{
    if (!fgets(buf, LINE_MAX, stdin))
    {
        if (feof(stdin)) // end of file indicator check
        {
            // ctrl-d pressed so don't print error
            return 1;
        }
        else
        {
            // actual error occurred
            perror("fgets failed");
            return 1;
        }
    }
    // ############## LLM Generated Code Begins ##############
    buf[strcspn(buf, "\n")] = '\0'; // replacing \n with \0, as fgets includes \n in it's string
    // ############## LLM Generated Code Ends ##############
    return 0;
}