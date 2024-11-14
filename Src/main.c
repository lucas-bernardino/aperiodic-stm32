#include <stdint.h>
#include "miros.h"

// Task visualization variables (for debugging or visualization)
uint32_t task1Visualizer = 0;
uint32_t task2Visualizer = 0;
uint32_t task3Visualizer = 0;

uint32_t aperiodic1Execution = 0;
uint32_t aperiodic2Execution = 0;

// Stack arrays for the tasks
uint32_t stackTask1[40];
uint32_t stackTask2[40];
uint32_t stackTask3[40];
uint32_t stack_idleThread[40];

// Thread control blocks
OSThread task1Thread;
OSThread task2Thread;
OSThread task3Thread;

// Function prototypes for periodic tasks
void task1();
void task2();
void task3();

// Aperiodic task example functions
void aperiodicTask1() {
    // Example of an aperiodic task
	aperiodic1Execution++;
}
// Aperiodic task example functions
void aperiodicTask2() {
    // Example of an aperiodic task
	aperiodic2Execution++;
}

void main() {
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
    addAperiodicTask(aperiodicTask1, 1 * TICKS_PER_SEC, 1);  // Aperiodic Task 1 arrives at T=5 and has cost 2
    addAperiodicTask(aperiodicTask2, 2 * TICKS_PER_SEC, 3); // Aperiodic Task 2 arrives at T=10 and has cost 3

    // Run the OS
    OS_run();
}

// Task 1: A periodic task
void task1() {
    while (1) {
        task1Visualizer++;
        TaskAction(&task1Thread, task1Thread.remainingTime);
    }
}

// Task 2: A periodic task
void task2() {
    while (1) {
        task2Visualizer++;
        TaskAction(&task2Thread, task2Thread.remainingTime);
    }
}

// Task 3: A periodic task
void task3() {
    while (1) {
        task3Visualizer++;
        TaskAction(&task3Thread, task3Thread.remainingTime);
    }
}
