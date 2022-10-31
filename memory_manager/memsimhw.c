//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2020
// Subject : Operating System (101511-3)
// Student Name: Lee Byeong Heon (이병헌)
// Student Number: B684031
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes
#define TRACES 1000000
#define TRUE 1
#define FALSE 0

struct pageFrame {
	int frameNumber;
	int valid;
	int pid;
	int pte;
	int len; // for frameQueue
	struct pageFrame* prev;
	struct pageFrame* next;
} *pageFrame, frameQueue;

void pop(struct pageFrame* item) {
	if (frameQueue.len == 0) {
		printf("Error: pop error, queue is empty\n");
		exit(1);
	}

	(item->prev)->next = item->next;
	(item->next)->prev = item->prev;
	item->prev = item->next = NULL;
	frameQueue.len--;
}

void push(struct pageFrame* item) {  // prev에 push
	if (frameQueue.len == 0) {
		frameQueue.prev = frameQueue.next = item;
		item->prev = item->next = &frameQueue;
		frameQueue.len++;
		return;
	}

	(frameQueue.prev)->next = item;
	item->prev = frameQueue.prev;
	item->next = &frameQueue;
	frameQueue.prev = item;
	frameQueue.len++;
}

struct pageTableEntry {
	int valid;
	int mappedFrame; // Default = -1
	struct pageTableEntry* secondPageTable;
};

struct procEntry {
	char* traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry* pageTable;
	FILE* tracefp;
} *procTable;

void oneLevelVMSim(int simType, int numProcess, int nFrame) {

	unsigned int virtualAddr; char rw;
	int VPN;
	struct pageTableEntry* PTE;
	int reps, i, j;

	for (i = 0; i < numProcess; i++) {
		procTable[i].pageTable = (struct pageTableEntry*)malloc(sizeof(struct pageTableEntry) * (1L << (VIRTUALADDRBITS - PAGESIZEBITS)));

		// initialize pageTable each process
		for (j = 0; j < (1L << (VIRTUALADDRBITS - PAGESIZEBITS)); j++) {
			procTable[i].pageTable[j].valid = FALSE;
			procTable[i].pageTable[j].mappedFrame = -1;
		}
	}

	for (reps = 0; reps < TRACES; reps++) {
		for (i = 0; i < numProcess; i++) {
			/* 트레이스 파일로부터 한 줄의 Virtual Address를 읽어오는 과정 */
			fscanf(procTable[i].tracefp, "%x %c", &virtualAddr, &rw);

			VPN = virtualAddr >> (PAGESIZEBITS);
			// 트레이스 값의 왼쪽 20Bits를 VPN으로 저장

			procTable[i].ntraces++;
			// ntraces값 증가


			/* 읽어온 virtual Address로 Page Table에 접근하는 과정 */
			PTE = &(procTable[i].pageTable[VPN]);

			/* Page Hit인 경우를 처리하는 과정 */
			if (PTE->valid == TRUE) {
				procTable[i].numPageHit++;

				// FIFO Case
				if (simType == 0) {
					// do nothing
				}

				// LRU Case
				if (simType == 1) {
					pop(&pageFrame[PTE->mappedFrame]);
					push(&pageFrame[PTE->mappedFrame]);
				}
			}

			/* Page Fault인 경우를 처리하는 과정 */
			if (PTE->valid == FALSE) {
				procTable[i].numPageFault++;

				// pageFrame에 여유가 있는 경우 (FIFO, LRU 알고리즘 모두 동일하게 동작)
				if (frameQueue.len != nFrame) {
					for (j = 0; j < nFrame; j++) {
						if (pageFrame[j].valid == FALSE) {
							pageFrame[j].valid = TRUE;
							pageFrame[j].pid = i;
							pageFrame[j].pte = VPN;
							PTE->mappedFrame = j;
							PTE->valid = TRUE;
							push(&pageFrame[j]);
							break;
						}
					}
				}

				// pageFrame에 여유가 없어 Replacement가 필요한 경우 (FIFO, LRU 알고리즘 모두 동일하게 동작)
				if (frameQueue.len == nFrame) {
					procTable[(frameQueue.next)->pid].pageTable[(frameQueue.next)->pte].valid = FALSE;
					PTE->mappedFrame = (frameQueue.next)->frameNumber;
					PTE->valid = TRUE;
					pop(frameQueue.next);
					push(&pageFrame[PTE->mappedFrame]);
					pageFrame[PTE->mappedFrame].pid = i;
					pageFrame[PTE->mappedFrame].pte = VPN;
				}
			}
		}
	}

	for (i = 0; i < numProcess; i++) {
		printf("**** %s *****\n", procTable[i].traceName);
		printf("Proc %d Num of traces %d\n", i, procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n", i, procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n", i, procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}

}

void twoLevelVMSim(int numProcess, int nFrame, int firstLevelBits) {
	unsigned int virtualAddr; char rw;
	int secondLevelBits = VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits;
	int VPN;
	struct pageTableEntry* oldPTE;
	struct pageTableEntry* newPTE;
	int PT1, PT2;
	int reps, i, j;
	
	/* 프로세스 당, Master Page Table를 초기화하는 과정*/
	for (i = 0; i < numProcess; i++) {
		procTable[i].pageTable = (struct pageTableEntry*)malloc(sizeof(struct pageTableEntry) * (1L << firstLevelBits));

		// initialize pageTable each process
		for (j = 0; j < (1L << firstLevelBits); j++) {
			procTable[i].pageTable[j].valid = FALSE;
			procTable[i].pageTable[j].mappedFrame = -1;
			procTable[i].pageTable[j].secondPageTable = NULL;
		}
	}


	for (reps = 0; reps < TRACES; reps++) {
		for (i = 0; i < numProcess; i++) {
			/* 트레이스 파일로부터 한 줄의 Virtual Address를 읽어오는 과정 */
			fscanf(procTable[i].tracefp, "%x %c", &virtualAddr, &rw);

			VPN = virtualAddr >> (PAGESIZEBITS);
			PT1 = VPN >> secondLevelBits;
			PT2 = (VPN << firstLevelBits) >> firstLevelBits;
			// 트레이스 값의 왼쪽 20Bits를 VPN으로 저장
			// virtualAddr에서 PT1은 masterPageTable의 인덱스로 쓰인다.
			// virtualAddr에서 PT2는 secondPageTalbe의 인덱스로 쓰인다.

			procTable[i].ntraces++;
			// ntraces값 증가

			oldPTE = procTable[frameQueue.next->pid].pageTable[]
			newPTE = &(procTable[i].pageTable[PT1]);

			if (newPTE->valid == FALSE) {
			// secondPageTable이 생성되어 있지 않은 경우
				newPTE->secondPageTable = (struct pageTableEntry*)malloc(sizeof(struct pageTableEntry) * (1L << secondLevelBits));
				newPTE->valid = TRUE;
				procTable[i].num2ndLevelPageTable++;

				// Frame에 여유가 있는 경우
				if (frameQueue.len != nFrame) {
					for (j = 0; j < nFrame; j++) {
						if (pageFrame[j].valid == FALSE) {
							pageFrame[j].valid = TRUE;
							pageFrame[j].pid = i;
							pageFrame[j].pte = VPN;
							newPTE->secondPageTable->mappedFrame = j;
							newPTE->secondPageTable->valid = TRUE;
							push(&pageFrame[j]);
							break;
						}
					}
				}

				// Frame에 여유가 없는 경우 -> LRU Replacement
				if (frameQueue.len == nFrame) {
					procTable[frameQueue.next->pid].pageTable
				}
			}

			if (newPTE->valid == TRUE) {
			// 이미 secondPageTable이 생성되어 있는 경우

				// Page Hit인 경우
				if ((newPTE->secondPageTable + PT2)->valid == TRUE) {
					// Frame에 여유가 있는 경우

					// Frame에 여유가 없는 경우 -> LRU Replacement
				}

				// Page Fault인 경우
				if ((newPTE->secondPageTable + PT2)->valid == FALSE) {
					// Frame에 여유가 있는 경우

					// Frame에 여유가 없는 경우 -> LRU Replacement
				}
			}
		}
	}
	

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}
/*
void invertedPageVMSim(...) {

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}
*/
// argv = memsim.out - simType - firstLevelBits - PhysicalMemorySizeBits - TraceFileNames...
int main(int argc, char* argv[]) {
	int i, j, c;
	int simType, firstLevelBits, phyMemSizeBits;
	int numProcess;
	long nFrame;

	if (argc < 5) {
		printf("Usage of %s: simType firstLevelBits PhysicalMemorySizeBits TraceFileNames ...\n", argv[0]);
		exit(1);
	}

	if (atoi(argv[3]) < 12) {
		printf("PhysicalMemorySizeBits %s should be larger than PageSizeBits 12\n", argv[3]);
		exit(1);
	}

	if (atoi(argv[2]) >= VIRTUALADDRBITS - PAGESIZEBITS) {
		printf("firstLevelBits %d is too Big for the 2nd level page system", argv[2]);
		exit(1);
	}
	
	simType = atoi(argv[1]);
	// 0 : One-Level Page Table System, FIFO Replacement
	// 1 : One-Level Page Table System, LRU  Replacement
	// 2 : Two-Level Page Table System, LRU  Replacement
	// 3 : Inverted  Page Table System, LRU  Replacement
	firstLevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);
	numProcess = argc - 4;
	nFrame = 1L<<(phyMemSizeBits - 12);

	pageFrame = (struct pageFrame*)malloc(sizeof(struct pageFrame) * nFrame);
	procTable = (struct procEntry*)malloc(sizeof(struct procEntry) * numProcess);	
	frameQueue.valid = FALSE;
	frameQueue.pid = -1;
	frameQueue.pte = -1;
	frameQueue.len = 0;
	frameQueue.prev = frameQueue.next = &frameQueue;
	

	// initialize procTable for Memory Simulations
	for (i = 0; i < numProcess; i++) {

		// opening a tracefile for the process
		printf("process %d opening %s\n", i, argv[4 + i]);
		procTable[i].tracefp = fopen(argv[4 + i], "r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...", argv[4 + i]);
			exit(1);
		}

		procTable[i].traceName = (char*)malloc(strlen(argv[4 + i]));
		strcpy(procTable[i].traceName, argv[4 + i]);
		procTable[i].pid = i;
		procTable[i].ntraces = 0;
		procTable[i].num2ndLevelPageTable = 0;
		procTable[i].numIHTConflictAccess = 0;
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numIHTNonNULLAcess = 0;
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
	}
	
	// initialize procTable for Memory Simulations
	for (i = 0; i < nFrame; i++) {
		pageFrame[i].frameNumber = i;
		pageFrame[i].valid = FALSE;
		pageFrame[i].pid = -1;
		pageFrame[i].pte = -1;
		pageFrame[i].len = -1;
		pageFrame[i].prev = pageFrame[i].next = NULL;
	}

	printf("Num of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));

	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, numProcess, nFrame);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, numProcess, nFrame);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim(numProcess, nFrame, firstLevelBits);
	}
	/*
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim(...);
	}
	*/
	return(0);
}

// memsim 2 10 32 ../mtraces/bzip.trace ../mtraces/gcc.trace
// a.out 0 10 32 ../mtraces/bzip.trace ../mtraces/gcc.trace