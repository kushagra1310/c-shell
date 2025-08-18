#include "../include/headerfiles.h"
#include "../include/vectorlib.h"
#include "../include/stringlib.h"
#include "../include/queue.h"
#include "../include/shell.h"
#include <errno.h>
int ping_function(int pid, int sig_no)
{
    sig_no%=32;
    if(kill((pid_t)pid,sig_no)==-1)
    {
        if(errno == ESRCH)
        printf("No such process found\n");
        return 1;
    }
    else
    {
        printf("Sent signal signal_number to process with pid %d\n",pid);
    }
    return 0;
}