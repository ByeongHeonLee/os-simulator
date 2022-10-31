// Submission Date : 2020.10.13 (TUE)
// Subject : Operating System (101511 - 3)
// CPU Schedule Simulator Homework
// Student Number : B684031
// Name : Lee Byeong Heon (이병헌)

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#define SEED 10

// Debugging On/Off
// #define DEBUG

// process states
#define S_IDLE 0
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;
	int len;		// for queue
	struct ioDoneEvent* prev;
	struct ioDoneEvent* next;
	/* Functions only for ioDoneEventQueue below:
	 * void pushIoQueue(struct ioDoneEvent* inputIoDoneEvent) 
	 * void popIoQueue()
	 * void sortIoQueue()
	 */
} ioDoneEventQueue, * ioDoneEvent;

void pushIoQueue(struct ioDoneEvent* inputIoDoneEvent) {
// To pop at ioDoneEventQueue's tail
	if (ioDoneEventQueue.len == 0) {
		ioDoneEventQueue.prev = ioDoneEventQueue.next = inputIoDoneEvent;
		inputIoDoneEvent->prev = inputIoDoneEvent->next = &ioDoneEventQueue;
		ioDoneEventQueue.len++;
		return;
	}

	inputIoDoneEvent->prev = ioDoneEventQueue.prev;
	(ioDoneEventQueue.prev)->next = inputIoDoneEvent;
	ioDoneEventQueue.prev = inputIoDoneEvent;
	inputIoDoneEvent->next = &ioDoneEventQueue;
	ioDoneEventQueue.len++;
}

void popIoQueue() {
// To pop at ioDoneEventQueue's head
	if (ioDoneEventQueue.len == 0)
		return;

	ioDoneEventQueue.next = (ioDoneEventQueue.next)->next;
	(ioDoneEventQueue.next)->prev = &ioDoneEventQueue;
	ioDoneEventQueue.len--;
}

void sortIoQueue() {
// ioDoneEventQueue must be sorted ascending order for doneTime
	if (ioDoneEventQueue.len < 2)
		return;

	int i, j, minVal, minIndex;
	struct ioDoneEvent* pointer = NULL;


	for (i = 0; i < ioDoneEventQueue.len; i++) {
		pointer = &ioDoneEventQueue;
		minVal = (pointer->next)->doneTime;
		minIndex = 0;

		for (j = 0; j < ioDoneEventQueue.len - i; j++) {
			pointer = pointer->next;
			if (pointer->doneTime < minVal) {
				minVal = pointer->doneTime;
				minIndex = j;
			}
		}

		pointer = &ioDoneEventQueue;
		for (j = 0; j < minIndex + 1; j++)
			pointer = pointer->next;

		(pointer->prev)->next = pointer->next;
		(pointer->next)->prev = pointer->prev;
		ioDoneEventQueue.len--;

		pushIoQueue(pointer);
	}
}

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process* prev;
	struct process* next;
} *procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process* runningProc;

void pushQueue(struct process* Queue, struct process* inputProc) {
// To push at head of readyQueue or blockedQueue
	if (Queue->len == 0) {
		Queue->prev = Queue->next = inputProc;
		inputProc->prev = inputProc->next = Queue;
		Queue->len++;
		return;
	}
		
	inputProc->prev = Queue->prev;
	(Queue->prev)->next = inputProc;
	Queue->prev = inputProc;
	inputProc->next = Queue;
	Queue->len++;
}

void popQueue(struct process* Queue, struct process *deleteProc) {
// To pop deleteProc in readyQueue or blockedQueue
	if (Queue->len == 0)
		return;

	struct process *pointer = Queue->next;
	
	while(pointer->id != deleteProc->id){
		if(pointer == Queue){
			printf("Target does not exist in Queue\n");
			return ;
		}

		pointer = pointer->next;
	}

	(pointer->prev)->next = pointer->next;
	(pointer->next)->prev = pointer->prev;
	Queue->len--;
}

int cpuReg0, cpuReg1;
int currentTime = 0;
int* procIntArrTime, * procServTime, * ioReqIntArrTime, * ioServTime;

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	
	// For debugging
	// printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
	int i;
	for (i = 0; i < NPROC; i++) {
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void procExecSim(void (*scheduler)()) {
	int pid, qTime = 0, cpuUseTime = 0, nproc = 0, termProc = 0, nioreq = 0;
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;

	int flag = 0;

	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];
	cpuReg0 = cpuReg1 = 0;
	
	idleProc.id = -1;
	idleProc.state = S_IDLE;
	idleProc.targetServiceTime = INT_MAX;
	runningProc = &idleProc;

	while (1) {

		currentTime++; 
		qTime++;

		if (runningProc->state != S_IDLE) {
			runningProc->serviceTime++;
			cpuUseTime++;
		}


		flag = 0;
		// if flag == 0, then runningProc don't be added in readyQueue
		// if flag == 1, then runningProc will be added in readyQueue

		// MUST CALL compute() Inside While loop
		compute();


		if (runningProc->serviceTime == runningProc->targetServiceTime) {
		// CASE : Process Terminate

			runningProc->state = S_TERMINATE;
			runningProc->endTime = currentTime;
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			termProc++;
			qTime = 0;

		} // End of CASE : Process Terminate


		 if (cpuUseTime == nextIOReqTime) {
		// CASE : IO Request (only when the process does not terminate)

			if (runningProc->state == S_RUNNING) {

				if(qTime != QUANTUM) // Is IO-Bound Process?
					runningProc->priority++;

				runningProc->state = S_BLOCKED;
				runningProc->saveReg0 = cpuReg0;
				runningProc->saveReg1 = cpuReg1;
				pushQueue(&blockedQueue, runningProc);
			}
	
			ioDoneEvent[nioreq].procid = runningProc->id;
			ioDoneEvent[nioreq].doneTime = currentTime + ioServTime[nioreq];
			pushIoQueue(&ioDoneEvent[nioreq]);
			sortIoQueue();

			nextIOReqTime += ioReqIntArrTime[++nioreq];
			qTime = 0;
			
		} // End of CASE : IO Request


		if (currentTime == nextForkTime) {
		// CASE : Fork

			procTable[nproc].state = S_READY;
			procTable[nproc].startTime = currentTime;
			pushQueue(&readyQueue, &procTable[nproc]);

			nextForkTime += procIntArrTime[++nproc];

			qTime = 0;
			flag = 1;

		} // End of CASE : Fork


		while (ioDoneEventQueue.next->doneTime == currentTime) {
		// CASE : IO Done Event

			// Processing for ioDoneEvnet
			pid = (ioDoneEventQueue.next)->procid;

			if (procTable[pid].serviceTime == procTable[pid].targetServiceTime) {
			// Does ioDoneEvent no longer have to be CPU-allocated?

				if (procTable[pid].state != S_TERMINATE) {
				// Not terminated IO done process
					procTable[pid].state = S_TERMINATE;
					procTable[pid].endTime = currentTime;
					runningProc->saveReg0 = cpuReg0;
					runningProc->saveReg1 = cpuReg1;
					termProc++;
					popIoQueue();
				} else{
				// Already terminated IO done process
					popIoQueue();
				}

			}	else {
			// Does ioDoneEvent have to be CPU-allocated?

				procTable[pid].state = S_READY;
				popIoQueue();
				popQueue(&blockedQueue, &procTable[pid]);
				pushQueue(&readyQueue, &procTable[pid]);
			}
			// End of processing for ioDoneEvent

			if( (runningProc->state == S_RUNNING) && (qTime == QUANTUM) )
			// Quantum expiration Case in ioDoneEvent Case will be don't care
				runningProc->priority++;

			qTime = 0;
			flag = 1;

		} // End of CASE : IO Done Event


		if (qTime == QUANTUM) {
		// CASE : The quantum expires 

			if (runningProc->state == S_RUNNING) {

				runningProc->priority--; // This Process is CPU-Bound Process
				flag = 1;
			}
			
			qTime = 0;

		} // End of CASE : The quantum expires


		if ( (runningProc->state == S_RUNNING) && (flag == 1) ) {
 
			runningProc->state = S_READY;
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			pushQueue(&readyQueue, runningProc);
			qTime = 0;
		}


		if (termProc == NPROC) // End condition of while loop
			break;

		if(runningProc->state != S_RUNNING) {
			scheduler();
			qTime = 0;
		}

	} // End of while loop
}


/* * * * * * * * * * * * * * * * *
 *     Scheduling Functions      * 
 * * * * * * * * * * * * * * * * */ 

// Round Robin Scheduling
void RRschedule() {
	
	if(readyQueue.len == 0) {
		runningProc = &idleProc;
		return;
	}

	runningProc = readyQueue.next;

	popQueue(&readyQueue, runningProc);
	runningProc->state = S_RUNNING;
	cpuReg0 = runningProc->saveReg0;
	cpuReg1 = runningProc->saveReg1;
}


// Shortest Job First Scheduling (Modified)
void SJFschedule() {

	if(readyQueue.len == 0) {
		runningProc = &idleProc;
		return;
	}

	int pid = (readyQueue.next)->id;
	int minTargetTime = (readyQueue.next)->targetServiceTime;
	struct process *pointer = readyQueue.next;

	while(pointer != &readyQueue){
		if(pointer->targetServiceTime < minTargetTime){
			minTargetTime = pointer->targetServiceTime;
			pid = pointer->id;
		}
		pointer = pointer->next;
	}

	popQueue(&readyQueue, &procTable[pid]);
	procTable[pid].state = S_RUNNING;
	cpuReg0 = procTable[pid].saveReg0;
	cpuReg1 = procTable[pid].saveReg1;
	runningProc = &procTable[pid];
}


// Shortest Remaining Time Next Scheduling (modified)
void SRTNschedule() {

	if(readyQueue.len == 0) {
		runningProc = &idleProc;
		return;
	}

	int pid = (readyQueue.next)->id;
	int minServiceTime = (readyQueue.next)->targetServiceTime - (readyQueue.next)->serviceTime;
	struct process *pointer = readyQueue.next;

	while(pointer != &readyQueue){
		if( (pointer->targetServiceTime - pointer->serviceTime) < minServiceTime){
			minServiceTime = (pointer->targetServiceTime - pointer->serviceTime);
			pid = pointer->id;
		}
		pointer = pointer->next;
	}

	popQueue(&readyQueue, &procTable[pid]);
	procTable[pid].state = S_RUNNING;
	cpuReg0 = procTable[pid].saveReg0;
	cpuReg1 = procTable[pid].saveReg1;
	runningProc = &procTable[pid];
}


// Guaranteed Scheduling (modified)
void GSschedule() {

	if(readyQueue.len == 0) {
		runningProc = &idleProc;
		return;
	}

	int pid = (readyQueue.next)->id;
	double minRatio = (double)((readyQueue.next)->serviceTime) / (double)((readyQueue.next)->targetServiceTime );
	struct process *pointer = readyQueue.next;

	while(pointer != &readyQueue){
		if ((double)(pointer->serviceTime) / (double)(pointer->targetServiceTime) < minRatio){
			minRatio = (double)(pointer->serviceTime) / (double)(pointer->targetServiceTime);
			pid = pointer->id;
		}
		pointer = pointer->next;
	}

	popQueue(&readyQueue, &procTable[pid]);
	procTable[pid].state = S_RUNNING;
	cpuReg0 = procTable[pid].saveReg0;
	cpuReg1 = procTable[pid].saveReg1;
	runningProc = &procTable[pid];
}


// Simple Feed Back Scheduling
void SFSschedule() {

	if(readyQueue.len == 0) {
		runningProc = &idleProc;
		return;
	}

	int pid = (readyQueue.next)->id;
	int maxPriority = (readyQueue.next)->priority;
	struct process *pointer = readyQueue.next;

	
	while(pointer != &readyQueue){
		if(maxPriority < pointer->priority){
			maxPriority = pointer->priority;
			pid = pointer->id;
		}
		pointer = pointer->next;
	}

	popQueue(&readyQueue, &procTable[pid]);
	procTable[pid].state = S_RUNNING;
	cpuReg0 = procTable[pid].saveReg0;
	cpuReg1 = procTable[pid].saveReg1;
	runningProc = &procTable[pid];
}


void printResult() {
// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime = 0, totalProcServTime = 0, totalIOReqIntArrTime = 0, totalIOServTime = 0;
	long totalWallTime = 0, totalRegValue = 0;
	for (i = 0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		   printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			 i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0 + procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue); */
	}
	for (i = 0; i < NPROC; i++) {
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for (i = 0; i < NIOREQ; i++) {
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}

	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float)totalProcIntArrTime / NPROC, (float)totalProcServTime / NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float)totalIOReqIntArrTime / NIOREQ, (float)totalIOServTime / NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC, NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float)totalWallTime / NPROC, (float)totalRegValue / NPROC);

}

int main(int argc, char* argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;

	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n", argv[0]);
		exit(1);
	}

	SCHEDULING_METHOD = atoi(argv[1]);
	// CPU Scheduling Algorithm
	// 1 : Round Robin
	// 2 : SJF
	// 3 : SRTN
	// 4 : Guaranteed Scheduling
	// 5 : Simple Feedback Scheduling

	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);

	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);

	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);

	srandom(SEED);

	// allocate array structures
	procTable = (struct process*)malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent*)malloc(sizeof(struct ioDoneEvent) * NIOREQ);

	procIntArrTime = (int*)malloc(sizeof(int) * NPROC);
	procServTime = (int*)malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int*)malloc(sizeof(int) * NIOREQ);
	ioServTime = (int*)malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;

	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len = readyQueue.len = blockedQueue.len = 0;

	// generate process interarrival times
	for (i = 0; i < NPROC; i++) {
		procIntArrTime[i] = random() % (MAX_INT_ARRTIME - MIN_INT_ARRTIME + 1) + MIN_INT_ARRTIME;
	}

	// assign service time for each process
	for (i = 0; i < NPROC; i++) {
		procServTime[i] = random() % (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];
	}

	ioReqAvgIntArrTime = totalProcServTime / (NIOREQ + 1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);

	// generate io request interarrival time
	for (i = 0; i < NIOREQ; i++) {
		ioReqIntArrTime[i] = random() % ((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME) * 2 + 1) + MIN_IOREQ_INT_ARRTIME;
	}

	// generate io request service time
	for (i = 0; i < NIOREQ; i++) {
		ioServTime[i] = random() % (MAX_IO_SERVTIME - MIN_IO_SERVTIME + 1) + MIN_IO_SERVTIME;
	}

#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for (i = 0; i < NPROC; i++) {
		printf("%d ", procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for (i = 0; i < NPROC; i++) {
		printf("%d ", procTable[i].targetServiceTime);
	}
	printf("\n");
#endif

	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n", MIN_IOREQ_INT_ARRTIME,
		(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME) * 2 + MIN_IOREQ_INT_ARRTIME);

#ifdef DEBUG		
	printf("IO Req Interarrival Time :\n");
	for (i = 0; i < NIOREQ; i++) {
		printf("%d ", ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for (i = 0; i < NIOREQ; i++) {
		printf("%d ", ioServTime[i]);
	}
	printf("\n");
#endif

	void (*schFunc)();

	switch (SCHEDULING_METHOD) {
		case 1: schFunc = RRschedule; break;
		case 2: schFunc = SJFschedule; break;
		case 3: schFunc = SRTNschedule; break;
		case 4: schFunc = GSschedule; break;
		case 5: schFunc = SFSschedule; break;
		default: printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}

	initProcTable();
	procExecSim(schFunc);
	printResult();

}