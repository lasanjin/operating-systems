## Blocking: Semaphores
 - A Semaphore is a non-negative integer variable with two operations
   - Down = P = Sleep = Wait
     - Checks if the semaphore's value is greater than 0 
       -  If it is, it decrements it and continues
       -  Otherwise, if the value is 0, the process is put to sleep and the down is (for the moment) not carried through
   - Up = V = Wakeup = Signal
     - Increments the Semaphore's value
     - If it was $0$ and $\geq 1$ processes are already trying to access it, it allows a process to carry out its down operation but does not decrement its value
   - Can be used for other things in addition to mutual exclusion
     - It is usually declared as **mutex**
       - A special type of Semaphore called a Binary Semaphore that has only two possible values (opposed to any non-negative Integer value)
         - 0 = unlocked
         - 1 = locked
       - Mutex is always initialized to 1
       - When a process wants to enter a critical section (CS), it does a down operation on the mutex to make it a 0
         - Any other process that tries to enter the CS will be blocked, because it is not allowed on a mutex Semaphore with a value of 0  
       - When it finishes its tasks in the CS, it does an up operation on the mutex to take it back to 1

## Critical Section
Critical section (CS) must ensure:
 - Mutual exclusion
   - One process/resource at any given time
 - Progress
   - No deadlock/livelock
   - Must enter CS in bounded time
 - Fairness
   - No starvation, i.e. bounded waiting

## Deadlock
Conditions for deadlock describes the four conditions necessary and sufficient to cause deadlock

 - Hold and wait
   - Hold resource
   - Ask for non-avaliable resource
 - Mutual exclusion
   - The resources involved must be unshareable
     - One process/resource at any given time
 - No pre-emption
   - Resource can only be released voulontarily
     - The processes must not have resources taken away while that resource is being used
 - Circluar wait
   - Dining philosophers problem
     - Each process holds resources which are currently being requested by the next process in the chain

## Dinig Philosophers
Originally formulated in 1965 by Edsger Dijkstra as a student exam exercise. 

 - Five silent philosophers sit at a round table with bowls of spaghetti
 - Forks are placed between each pair of adjacent philosophers
 - Each philosopher must alternately think and eat
 - A philosopher can only eat spaghetti when they have both left and right forks
 - Each fork can be held by only one philosopher and so a philosopher can use the fork only if it is not being used by another philosopher
 - After an individual philosopher finishes eating, they need to put down both forks so that the forks become available to others
 - A philosopher can take the fork on their right or the one on their left as they become available, but cannot start eating before getting both forks 

Eating is not limited by the remaining amounts of spaghetti or stomach space; an infinite supply and an infinite demand are assumed. 

### Algorithm

```cpp
int N; // # philosophers
int LEFT = (i+1) % N // i = current philosopher
int RIGHT = (i + (N-1)) % N
int HUNGRY = 0
int EATING = 1
int THINKING = 2
int state[N];
semaphore S[N];
semaphore mutex;
```

```cpp
void philosophers(int i) {
    think();
    take_fork();
    eat();
    put_fork();
}
```

```cpp
void take_fort(int i) {
    down(&mutex); // enter critical section (CS) (prevent deadlock)
    state[i] = HUNGRY;
    test(i);
    up(&mutex); // leave CS
    down(&S[i]); // sleep if test did not pass
}
```

```cpp
void test(int i) {
    if (state[i] == HUNGRY && state[LEFT] == THINKING && state[RIGHT] == THINKING) {
        state[i] = EATING;
        up(&S[i]) //
    }
}
```

```cpp
void put_fork() {
    down(&mutex) // enter CS
    state[i] = THINKING;
    test(LEFT); // test if left neighbor want to eat (prevent stavation)
    test(RIGHT); // 
    up(&mutex); // leave CS
}
```



