# Operating Systems

## Description
Lab assignments and some [notes](notes/NOTES.md) from Operating Systems course at Chalmers.

### shell
The purpose of this assignment was to implement a command **shell**, able to run programs on Linux platforms and supporting basic IO redirection and piping between commands.

### timer and batch-scheduler
In this assignment we used Pintos, an educational operating system supporting kernel threads, loading, and running of user programs and a file system.

- **timer**
  - The purpose of this task was to enhance the synchronization implementations. One of the classic synchronization methods for a thread is busy-waiting, i.e. spinning. Our goal was to provide an alternative implementation of a sleep function.
- **batch-scheduler**
  - The purpose of this task was to solve a simple batch scheduling problem. More specifically, we handled the synchronization issues that arise when scheduling different batches of jobs. 
    - The assumption was that our system was extended with an external processing accelerator (e.g. a GPU or a co-processor) with X Processing Units (PUs). 
    - Each task was handled by one thread each, and contain the appropriate data/results from/to the accelerator. 
    - The communication bus with the accelerator was half duplex (i.e. one direction can be used at a time) and has limited bandwidth as only 3 slots can be used by tasks at a time.
