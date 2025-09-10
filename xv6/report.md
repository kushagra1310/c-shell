# PART A
## SYSCALL IMPLEMENTATION
Just added an extra global variable to keep track of the bytes read, made a new function in the kernel which returns the variable.
Added the number associated with the syscall in the respective header file. Used uint64 so value wraps around on it's own.

## USER PROGRAM
Wrote the program in the user directory and read 100 bytes from a random file. Syscall called twice.

# PART B
## FCFS
Added extra field in the process struct for tracking arrival time. When scheduler wants to decide it scans through the process array and chooses the one with the earliest arrival time. After that schedule that process and move on.
## CFS
Added extra fields in the process struct for tracking time slice, runtime and ticks ran till now. Made corresponding changes in the clockintr() function and usertrap() and kerneltrap() functions as well, so that the interrupt raised by yield occours only after the time slice is consumed by the process.
## LOGGING
Didn't implement as printf statements messed up the scheduler completely due to their slow nature. A function like scheduler can't handle slow calls like it, so logging wasn't practically possible while ensuring efficiency of the system.