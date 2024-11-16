#include <stdint.h>
#include "miros.h"

// For debugging periodic tasks
uint32_t task1Visualizer = 0;
uint32_t task2Visualizer = 0;
uint32_t task3Visualizer = 0;

// For debugging periodic tasks
int32_t aperiodicExecution = -1;
int32_t aperiodicExecution2 = -1;

// Shared resource variable
int32_t resource = 0;

// Variables that will be used to simulate heavy computation
volatile int counter, j;

// Stack arrays for the tasks
uint32_t stackTask1[40];
uint32_t stackTask2[40];
uint32_t stackTask3[40];
uint32_t stack_idleThread[40];

// Thread control blocks
OSThread task1Thread;
OSThread task2Thread;
OSThread task3Thread;

// Prototypes
void task1();
void task2();
void task3();
void aperiodicTask();
void aperiodicTask2();

// Mutex that will be used when accessing critical section.
semaphore mutex;

int main() {
    OS_init(stack_idleThread, sizeof(stack_idleThread));

    OSThread_start(&task1Thread, 5U, &task1, stackTask1, sizeof(stackTask1),
                   3 * TICKS_PER_SEC, 5 * TICKS_PER_SEC);

    OSThread_start(&task2Thread, 2U, &task2, stackTask2, sizeof(stackTask2),
                   1 * TICKS_PER_SEC, 8 * TICKS_PER_SEC);

    OSThread_start(&task3Thread, 1U, &task3, stackTask3, sizeof(stackTask3),
                   1 * TICKS_PER_SEC, 10 * TICKS_PER_SEC);

    // Aperiodic task with arrival at T = 1 and cost of C = 1
    addAperiodicTask(aperiodicTask, 1 * TICKS_PER_SEC, 1 * TICKS_PER_SEC);

    // Aperiodic task with arrival at T = 4 and cost of C = 1
    addAperiodicTask(aperiodicTask2, 4 * TICKS_PER_SEC, 1 * TICKS_PER_SEC);

    sem_init(&mutex, 1);

    OS_run();
}

void task1() {
    while (1) {
    	sem_wait(&mutex, &task1Thread);
        resource = resource + 5;
    	sem_post(&mutex, &task1Thread);
        TaskAction(&task1Thread, task1Thread.remainingTime, &task1Visualizer);
    }
}

void task2() {
    while (1) {
        TaskAction(&task2Thread, task2Thread.remainingTime, &task2Visualizer);
    }
}

void task3() {
    while (1) {
    	sem_wait(&mutex, &task3Thread);
        resource = resource - 5;
        // Simulating heavy computation, where task3 is taking a lot of time...
        for (counter = 0; counter < 1000000; counter++) {
            j = counter * counter;
        }
        sem_post(&mutex, &task3Thread);
        TaskAction(&task3Thread, task3Thread.remainingTime, &task3Visualizer);
    }
}

void aperiodicTask() {
	aperiodicExecution++;
}

void aperiodicTask2() {
	aperiodicExecution2++;
}
