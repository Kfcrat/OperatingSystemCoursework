/**************************************************************************************************************************
			- DO NOT CHANGE THE CONTENTS OF THIS FILE FOR YOUR COURSEWORK (APART FROM THE VALUES FOR THE CONSTANTS)
			- ONLY WORK WITH THE OFFICIAL VERSION
**************************************************************************************************************************/

#include <sys/time.h>
#include <math.h>
#include <pthread.h>

typedef enum {false, true} bool;

#define SIZE_OF_PROCESS_TABLE 16 // number of elements in the process table
#define NUMBER_OF_PROCESSES 10// number of elements to simulate
#define MAX_CONCURRENT_PROCESSES 2 // max number of processes in the system at any one point in time

#define TIME_SLICE 5 // Duration of the time slice for the round robin algorithm
#define MAX_BURST_TIME 50 // Max CPU time that any one process requires

#define NUMBER_OF_PAGES 4096 // Number of entries in the page table
#define NUMBER_OF_FRAMES 512 // Number of frames
#define PAGE_SIZE 256  // Size of a page / frame

// process states
#define READY 1
#define RUNNING 2
#define PAGE_FAULTED 3
#define TERMINATED 4

// PAGING
#define FIFO_PAGING 1
#define NRU_PAGING 2
#define PAGING FIFO_PAGING

// struct defining a memory address
typedef struct {
  int iPage;
  int iOffset;
} MemoryAddress;

// Macro to initialise memory address
#define MEMORY_ADDRESS_INITIALIZER {.iPage = -1, .iOffset = -1}

// Entry in the page table / frame table
typedef struct {
  int iPID;
  int iPage;
  int iFrame;
  int iAccessed;
  int iDirty;
  pthread_mutex_t m;
} FrameEntry;

// Macro to initialise a page entry
#define PAGE_TABLE_ENTRY_INITIALIZER {.iPID = -1, .iPage = -1, .iFrame = -1, .iAccessed = 1, .iDirty = 0, .m = PTHREAD_MUTEX_INITIALIZER}

// struct representing a (simplified) process control block
typedef struct {
  struct timeval oTimeCreated;
  struct timeval oFirstTimeRunning;
  struct timeval oLastTimeRunning;

  int iPID; // process identifier, assumed to be positive int up to MAX_VALUE
  int iHash;  // hash code (based on process identifier)

  int iBurstTime; // Initial CPU time required by the process
  int iRemainingBurstTime; // CPU time left for the process

  int iStatus; // process state

  FrameEntry * apPageTable[NUMBER_OF_PAGES]; // page table (contains pointers to PageTableEntries)

  int iLocality; // current locality
  int iWidth;  // width of the locality
  int iPageFaults;

  MemoryAddress oLastRequested; // last requested address

} Process;

// Macro to initialise a process struct (Process control block)
#define PROCESS_INITIALIZER {.iPID = -1, .iHash = -1, .iBurstTime = 0, .iRemainingBurstTime = 0, .apPageTable = {[0 ... NUMBER_OF_PAGES - 1] = NULL}, .iLocality = 0, .iWidth = 1, .oLastRequested = { .iPage = -1, .iOffset = -1}, .iStatus = READY, .iPageFaults = 0}

// creates a process control block in dynamic memory. initialises it, and returns it
Process * generateProcess(int iPID);

// simulates a process running in round robin
void runPreemptiveProcess(Process * pTemp, bool bSimulatePageFaults);

// function to calculate the difference between two time stamps
long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);

// function to map a dummy frame into the process table of the process
void mapDummyFrame(Process * pProcess);

// function to map the given frame into the process table of the process
void mapFrame(Process * pProcess, FrameEntry * pEntry);

// function to re-claim a frame from a process, either through page reclaiming or when the process has finished
void reclaimFrame(FrameEntry * pEntry);

