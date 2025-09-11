# Scheduling Algorithms Performance Comparison Report

**# PART A**
**## SYSCALL IMPLEMENTATION**
Just added an extra global variable to keep track of the bytes read, made a new function in the kernel which returns the variable.
Added the number associated with the syscall in the respective header file. Used uint64 so value wraps around on it's own.

**## USER PROGRAM**
Wrote the program in the user directory and read 100 bytes from a random file. Syscall called twice.

**# PART B**

**## ROUND ROBIN**
The default Round Robin scheduler uses a fixed time quantum. Each process runs for its allocated time slice before being preempted and moved to the back of the ready queue. Implementation involved maintaining a circular queue structure and implementing timer-based preemption logic to ensure fair CPU allocation among processes.

=== TEST RESULTS ===
Total test time: 282 ticks
Average time per process: 28 ticks
=== DETAILED PROCESS STATISTICS ===
PID State CTime RTime WTime Turnaround
1   2     0     0     1     0
2   2     1     0     0     473
3   4     223   0     8     251

**## FCFS**
Added extra field in the process struct for tracking arrival time. When scheduler wants to decide it scans through the process array and chooses the one with the earliest arrival time. After that schedule that process and move on.

Total test time: 325 ticks
Average time per process: 32 ticks
=== DETAILED PROCESS STATISTICS ===
PID State CTime RTime WTime Turnaround
1   2     0     1     1     0
2   2     2     0     0     785
3   4     462   0     0     325

**## CFS**
Added extra fields in the process struct for tracking time slice, runtime and ticks ran till now. Made corresponding changes in the clockintr() function and usertrap() and kerneltrap() functions as well, so that the interrupt raised by yield occours only after the time slice is consumed by the process.
For the scheduling part, instead of keeping them all in ascending order, which would take o(n) in each iteration anyway (to shift processes after one is done), I'm repeatedly scanning and bringing the job with the least vruntime to the front.

=== TEST RESULTS ===
Total test time: 258 ticks
Average time per process: 25 ticks
=== DETAILED PROCESS STATISTICS ===
PID State CTime RTime WTime Turnaround
1   2     0     32    8     40
2   2     1     29    12    42
3   4     45    38    6     89

**## LOGGING**
Didn't implement as printf statements messed up the scheduler completely due to their slow nature. A function like scheduler can't handle slow calls like it, so logging wasn't practically possible while ensuring efficiency of the system.

## Performance Comparison Results

### Summary of Performance Metrics

| Scheduling Policy | Total Test Time | Average Time per Process |
|-------------------|----------------|-------------------------|
| CFS               | 258 ticks      | 25 ticks               |
| Round Robin       | 282 ticks      | 28 ticks               |
| FCFS              | 325 ticks      | 32 ticks               |

## Analysis

**CFS Performance:** CFS showed the best overall performance with the lowest total test time (258 ticks) and average time per process (25 ticks). This is expected as CFS aims to provide fair CPU allocation while minimizing latency by selecting processes with the least virtual runtime.

**Round Robin Performance:** Round Robin showed moderate performance with a total test time of 282 ticks and average of 28 ticks per process. 

**FCFS Performance:** FCFS showed the highest total test time (325 ticks) and average time per process (32 ticks), due to the convoy effect where processes must wait for earlier arrivals to complete regardless of their execution requirements.

## Conclusions

The performance comparison clearly demonstrates that CFS provides superior performance for the given workload, offering the best efficiency in terms of total execution time. Round Robin maintains consistent performance across processes but with moderate overhead, while FCFS, despite its implementation simplicity, suffers from poor performance due to lack of preemption capabilities and the convoy effect.