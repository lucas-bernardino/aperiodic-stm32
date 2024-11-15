#include <stdint.h>
#include "miros.h"

// Task visualization variables (for debugging or visualization)
uint32_t task1Visualizer = 0;
uint32_t task2Visualizer = 0;
uint32_t task3Visualizer = 0;

uint32_t aperiodicExecution = 0;

int32_t resource = 0;
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

// Function prototypes for tasks
void task1();
void task2();
void task3();
void aperiodicTask();

semaphore mutex;

int main() {
    // Initialize the OS with idle thread stack
    OS_init(stack_idleThread, sizeof(stack_idleThread));

    // Start periodic tasks
    OSThread_start(&task1Thread, 5U, &task1, stackTask1, sizeof(stackTask1),
                   3 * TICKS_PER_SEC, 5 * TICKS_PER_SEC);

    OSThread_start(&task2Thread, 2U, &task2, stackTask2, sizeof(stackTask2),
                   1 * TICKS_PER_SEC, 8 * TICKS_PER_SEC);

    OSThread_start(&task3Thread, 1U, &task3, stackTask3, sizeof(stackTask3),
                   1 * TICKS_PER_SEC, 10 * TICKS_PER_SEC);

    // Add aperiodic tasks with specific arrival times and execution costs
    addAperiodicTask(aperiodicTask, 1 * TICKS_PER_SEC, 1);  // Aperiodic Task 1 arrives at T=5 and has cost 2

    sem_init(&mutex, 1);
    // Run the OS
    OS_run();
}

void task1() {
    while (1) {
        task1Visualizer++;
    	sem_wait(&mutex, &task1Thread);
        resource = resource + 5;
    	sem_post(&mutex, &task1Thread);
        TaskAction(&task1Thread, task1Thread.remainingTime);
    }
}

void task2() {
    while (1) {
        task2Visualizer++;
        TaskAction(&task2Thread, task2Thread.remainingTime);
    }
}

void task3() {
    while (1) {
        task3Visualizer++;
    	sem_wait(&mutex, &task3Thread);
        resource = resource - 5;
        // Simulando computacao pesada. Task 3 está tomando tempo dentro da zona critica
        for (counter = 0; counter < 1000000; counter++) {
            j = counter * counter;
        }
        sem_post(&mutex, &task3Thread);
    	TaskAction(&task3Thread, task3Thread.remainingTime);
    }
}

void aperiodicTask() {
	aperiodicExecution++;
}
