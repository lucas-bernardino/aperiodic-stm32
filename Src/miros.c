/****************************************************************************
* MInimal Real-time Operating System (MiROS), GNU-ARM port.
* version 1.26 (matching lesson 26, see https://youtu.be/kLxxXNCrY60)
*
* This software is a teaching aid to illustrate the concepts underlying
* a Real-Time Operating System (RTOS). The main goal of the software is
* simplicity and clear presentation of the concepts, but without dealing
* with various corner cases, portability, or error handling. For these
* reasons, the software is generally NOT intended or recommended for use
* in commercial applications.
*
* Copyright (C) 2018 Miro Samek. All Rights Reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*
* Git repo:
* https://github.com/QuantumLeaps/MiROS
****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "miros.h"
#include "qassert.h"
#include "stm32f1xx.h"

Q_DEFINE_THIS_FILE

OSThread * volatile OS_curr; /* pointer to the current thread */
OSThread * volatile OS_next; /* pointer to the next thread to run */

OSThread *OS_thread[32 + 1]; /* array of threads started so far */
uint32_t OS_readySet; /* bitmask of threads that are ready to run */
uint32_t OS_delayedSet; /* bitmask of threads that are delayed */

uint32_t OSTotalTicks; // Total number of ticks counter

#define LOG2(x)        (32U - __builtin_clz(x))
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

uint32_t const MAX_VAL = UINT32_MAX;


AperiodicTask aperiodicTaskQueue[MAX_APERIODIC_TASKS];
uint32_t aperiodicTaskCount = 0;

uint32_t lowestPeriodTask = 0; // lowest period = highest priority

void addAperiodicTask(void (*taskHandler)(void), uint32_t arrivalTime, uint32_t cost) {
    if (aperiodicTaskCount < MAX_APERIODIC_TASKS) {
        aperiodicTaskQueue[aperiodicTaskCount].taskHandler = taskHandler;
        aperiodicTaskQueue[aperiodicTaskCount].arrivalTime = arrivalTime;
        aperiodicTaskQueue[aperiodicTaskCount].remainingCost = cost;
        aperiodicTaskCount++;

        // Sort the tasks by arrival time (bubble sort for simplicity)
        for (uint32_t i = 0; i < aperiodicTaskCount - 1; i++) {
            for (uint32_t j = i + 1; j < aperiodicTaskCount; j++) {
                if (aperiodicTaskQueue[i].arrivalTime > aperiodicTaskQueue[j].arrivalTime) {
                    AperiodicTask temp = aperiodicTaskQueue[i];
                    aperiodicTaskQueue[i] = aperiodicTaskQueue[j];
                    aperiodicTaskQueue[j] = temp;
                }
            }
        }
    }
}


OSThread idleThread;
void main_idleThread() {
    while (1) {
        OS_onIdle();
    }
}

// Function to execute the aperiodic tasks that are ready
void executeAperiodicTasks() {
    for (uint32_t i = 0; i < aperiodicTaskCount; i++) {
        AperiodicTask *task = &aperiodicTaskQueue[i];

        // Ensure the task is ready to execute (arrival time <= OSTotalTicks) and has remaining cost
        if (task->arrivalTime <= OSTotalTicks && task->remainingCost > 0) {
            // Check if no periodic task is running (i.e., we are in idle time)
            if (OS_curr == &idleThread) {
                task->taskHandler(); // Execute the task for one tick
                task->remainingCost--; // Decrease the remaining cost

                // If the task is finished, reset its arrival time to MAX_VAL
                if (task->remainingCost == 0) {
                    task->arrivalTime = MAX_VAL;
                }
                return;  // Exit after executing one tick of an aperiodic task
            }
        }
    }
}

// Check if there is aperiodic work to do during idle time
void checkForIdleAndAperiodicTasks() {
    // Only execute aperiodic tasks when no periodic tasks are active (idle time)
    if (OS_curr == &idleThread) {
        executeAperiodicTasks();  // Execute aperiodic tasks if any are ready
    }
}


void OS_init(void *stkSto, uint32_t stkSize) {
    /* set the PendSV interrupt priority to the lowest level 0xFF */
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);

    /* start idleThread thread */
    OSThread_start(&idleThread, 0U, &main_idleThread, stkSto, stkSize, 0U, 0U);

    OSTotalTicks = 0;
}

void checkLowestPriorityThread(OSThread** lowestPeriodThread, uint32_t* minimumPeriod) {
    *minimumPeriod = MAX_VAL;
    for(uint32_t i = 1; i < ARRAY_SIZE(OS_thread); i++) {
        if(OS_thread[i] && OS_thread[i]->Ti < *minimumPeriod) {
            *minimumPeriod = OS_thread[i]->Ti;
            *lowestPeriodThread = OS_thread[i];
        }
    }
}

void checkCompletedTask() {
    for(uint32_t i = 1; i < ARRAY_SIZE(OS_thread); i++){
    	if(OS_thread[i] && OSTotalTicks%OS_thread[i]->Ti == 0){
    		OS_thread[i]->isActive = true;
    		OS_thread[i]->remainingTime = OS_thread[i]->Ci;
    	}
    }
}

void chooseNextThread(OSThread** next, uint32_t* nextPeriod) {
    *nextPeriod = MAX_VAL;
    for(uint32_t i = 1; i < ARRAY_SIZE(OS_thread); i++) {
        if(OS_thread[i] && OS_thread[i]->isActive && (OS_thread[i]->Ti < *nextPeriod && OS_thread[i]->Ti > OS_curr->Ti)) {
            *nextPeriod = OS_thread[i]->Ti;
            *next = OS_thread[i];
        }
    }
}

void OS_sched(void) {
    /* choose the next thread to execute... */
    OSThread *next = OS_thread[0];
    OSThread *lowestPeriodThread;

    uint32_t minimumPeriod;

    checkLowestPriorityThread(&lowestPeriodThread, &minimumPeriod);
    checkCompletedTask();

    uint32_t compareTicksPeriod = OSTotalTicks % lowestPeriodThread->Ti;

    if(compareTicksPeriod == 0 || OSTotalTicks == 0 || lowestPeriodThread->isActive){
    	next = lowestPeriodThread;
    }
    else if(OS_curr->isActive){
    	next = OS_curr;
    }
    else if(!OS_curr->isActive){
        uint32_t nextPeriod;
        chooseNextThread(&next, &nextPeriod);
    }

    // Check for idle periods and run aperiodic tasks
    checkForIdleAndAperiodicTasks();


    /* trigger PendSV, if needed */
    if (next != OS_curr) {
        OS_next = next;
        //*(uint32_t volatile *)0xE000ED04 = (1U << 28);
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        __asm volatile("dsb");
//        __asm volatile("isb");
    }
    /*
     * DSB - whenever a memory access needs to have completed before program execution progresses.
     * ISB - whenever instruction fetches need to explicitly take place after a certain point in the program,
     * for example after memory map updates or after writing code to be executed.
     * (In practice, this means "throw away any prefetched instructions at this point".)
     * */
}

uint32_t getLowestPeriod() {
	uint32_t _lowestPeriod = OS_thread[1]->Ti;
    for(uint32_t i = 1; i < ARRAY_SIZE(OS_thread); i++) {
        if (OS_thread[i]->Ti < _lowestPeriod) {
        	_lowestPeriod = OS_thread[i]->Ti;
        }
    }
    return _lowestPeriod;
}

void OS_run() {
    /* callback to configure and start interrupts */
    OS_onStartup();

    lowestPeriodTask = getLowestPeriod();

    __disable_irq();
    OS_sched();
    __enable_irq();

    /* the following code should never execute */
    Q_ERROR();
}

void OS_tick(void) {
    uint32_t workingSet = OS_delayedSet;
    while (workingSet != 0U) {
        OSThread *t = OS_thread[LOG2(workingSet)];
        uint32_t bit;
        Q_ASSERT((t != (OSThread *)0) && (t->timeout != 0U));

        bit = (1U << (t->prio - 1U));
        --t->timeout;
        if (t->timeout == 0U) {
            OS_readySet   |= bit;  /* insert to set */
            OS_delayedSet &= ~bit; /* remove from set */
        }
        workingSet &= ~bit; /* remove from working set */
    }

    // Each OS_tick must increase our own TotalTicks variable
    OSTotalTicks++;
}

void OS_delay(uint32_t ticks) {
    uint32_t bit;
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idleThread */
    Q_REQUIRE(OS_curr != OS_thread[0]);

    OS_curr->timeout = ticks;
    bit = (1U << (OS_curr->prio - 1U));
    OS_readySet &= ~bit;
    OS_delayedSet |= bit;
    OS_sched();
    __asm volatile ("cpsie i");
}

void OSThread_start(
    OSThread *me,
    uint8_t prio, /* thread priority */
    OSThreadHandler threadHandler,
    void *stkSto, uint32_t stkSize,
	uint32_t Ci, uint32_t Ti)
{

    me->Ci = Ci;
    me->Ti = Ti;
    me->startupTi = Ti;
    me->remainingTime = Ci;
    me->isActive = true;


    /* round down the stack top to the 8-byte boundary
    * NOTE: ARM Cortex-M stack grows down from hi -> low memory
    */
    uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    /* priority must be in ragne
    * and the priority level must be unused
    */
    Q_REQUIRE((prio < Q_DIM(OS_thread))
              && (OS_thread[prio] == (OSThread *)0));

    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the thread's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }

    /* register the thread with the OS */
    OS_thread[prio] = me;
    me->prio = prio;
    /* make the thread ready to run */
    if (prio > 0U) {
        OS_readySet |= (1U << (prio - 1U));
    }
}

void TaskAction(OSThread *task, uint32_t remainingTime){
	uint32_t ticksPassed = OSTotalTicks;
	while(remainingTime > 0){
		task->remainingTime--;
		if(task->remainingTime == 0){
			task->isActive = false;
		}
		while(ticksPassed == OSTotalTicks) {
            // Do nothing
        }
		ticksPassed = OSTotalTicks;
		remainingTime--;
	}
}

void sem_init(semaphore* s, int32_t initValue) {
	Q_ASSERT(s);
	s->semCount = initValue;
	s->isBlocked = false;
}

void sem_wait(semaphore* s, OSThread* taskCaller) {
	Q_ASSERT(s);
	Q_ASSERT(taskCaller);
	__disable_irq();
	if (s->semCount == 0) {
		s->isBlocked = true;
	}
	while (s->semCount == 0) {
		OS_delay(1);
		OS_sched();
		if (s->isBlocked == false) {
			break;
		}
	}
	s->semCount--;
	OS_sched();

	taskCaller->Ti = lowestPeriodTask - 1;
}

void sem_post(semaphore* s, OSThread* taskCaller) {
	Q_ASSERT(s);
	__disable_irq();
	s->semCount++;
	if (s->isBlocked == true) {
		s->isBlocked = false;
	}
	__enable_irq();
	OS_sched();

	taskCaller->Ti = taskCaller->startupTi;
}

__attribute__ ((naked, optimize("-fno-stack-protector")))
void PendSV_Handler(void) {
__asm volatile (

    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSThread *)0) { */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,PendSV_restore \n"

    /*     push registers r4-r11 on the stack */
    "  PUSH          {r4-r11}          \n"

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    /* } */

    "PendSV_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=OS_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
    "  POP           {r4-r11}          \n"

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next thread */
    "  BX            lr                \n"
    );
}
