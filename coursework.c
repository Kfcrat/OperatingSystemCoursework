/**************************************************************************************************************************
      DO NOT CHANGE THE CONTENTS OF THIS FILE FOR YOUR COURSEWORK. ONLY WORK WITH THE OFFICIAL VERSION
**************************************************************************************************************************/
#include <stdlib.h>
#include <sys/time.h>
#include "coursework.h"
#include <stdio.h>
#include <pthread.h>

int getLocality() {
  return rand() % NUMBER_OF_PAGES;
}

int getLocalityWidth() {
  return (int) (rand() % (NUMBER_OF_FRAMES / MAX_CONCURRENT_PROCESSES)) * 1.2 + 1;
}

MemoryAddress generateAddress(Process * pProcess) {
  MemoryAddress oAddress;
  oAddress.iPage = pProcess->iLocality + rand() % pProcess->iWidth - pProcess->iWidth / 2;
  oAddress.iPage = oAddress.iPage < 0 ? 0 : (oAddress.iPage >= NUMBER_OF_PAGES ? NUMBER_OF_PAGES - 1 : oAddress.iPage);
  oAddress.iOffset = rand() % PAGE_SIZE;
  return oAddress;
}

Process * generateProcess(int iPID)
{  
  Process * pProcess = (Process *) malloc (sizeof(Process));
  if(pProcess == NULL)
  {
    printf("Malloc Failed\n");
    exit(-1);
  }
  *pProcess = (Process) PROCESS_INITIALIZER;
  gettimeofday(&pProcess->oTimeCreated, NULL);
  pProcess->iPID = iPID;
  pProcess->iHash = iPID % SIZE_OF_PROCESS_TABLE; 
  pProcess->iRemainingBurstTime = pProcess->iBurstTime = (rand() % MAX_BURST_TIME) + 1;
  pProcess->iLocality = getLocality();
  pProcess->iWidth = getLocalityWidth();
  return pProcess;
}

void simulateMemoryAccess(Process * pProcess, bool bSimulatePageFaults) {
  if(rand() % 200000 == 0) {
    pProcess->iLocality = getLocality();
    pProcess->iWidth = getLocalityWidth();
  }
  pProcess->oLastRequested = generateAddress(pProcess);
  if(bSimulatePageFaults) {
    if(pProcess->apPageTable[pProcess->oLastRequested.iPage] == NULL) {
      pProcess->iStatus = PAGE_FAULTED;
      pProcess->iPageFaults++;
      return;
    }
    pthread_mutex_lock(&pProcess->apPageTable[pProcess->oLastRequested.iPage]->m);
    if(pProcess->apPageTable[pProcess->oLastRequested.iPage]->iPID != pProcess->iPID || pProcess->apPageTable[pProcess->oLastRequested.iPage]->iPage == -1) {
      pProcess->iStatus = PAGE_FAULTED;
      pProcess->iPageFaults++;
    }
    pthread_mutex_unlock(&pProcess->apPageTable[pProcess->oLastRequested.iPage]->m);
    pProcess->apPageTable[pProcess->oLastRequested.iPage]->iAccessed = 1;
    if(rand() % 100000 == 0) {
      pProcess->apPageTable[pProcess->oLastRequested.iPage]->iDirty = 1;
    }
  }
}

void runProcess(Process * pProcess, int iBurstTime, bool bSimulatePageFaults) {
  if(pProcess->iBurstTime == pProcess->iRemainingBurstTime) 
    gettimeofday(&pProcess->oFirstTimeRunning, NULL);

  long int iDifference = 0;
  struct timeval oStartTime, oCurrentTime;
  gettimeofday(&oStartTime, NULL);
  
  do {     
    gettimeofday(&oCurrentTime, NULL);
    iDifference = getDifferenceInMilliSeconds(oStartTime, oCurrentTime);
    simulateMemoryAccess(pProcess, bSimulatePageFaults);
    if(pProcess->iStatus == PAGE_FAULTED) {
      pProcess->iRemainingBurstTime -= iDifference > iBurstTime ? iBurstTime : iDifference;
      return;
    }
  } while(iDifference < iBurstTime);
  pProcess->iRemainingBurstTime -= iDifference > iBurstTime ? iBurstTime : iDifference;
  if(pProcess->iRemainingBurstTime == 0) {
    pProcess->iStatus = TERMINATED;
    gettimeofday(&pProcess->oLastTimeRunning, NULL);
    return;
  }
  pProcess->iStatus = READY;
  return;
}

void runPreemptiveProcess(Process * pProcess, bool bSimulatePageFaults)
{
  int iBurstTime = pProcess->iRemainingBurstTime > TIME_SLICE ? TIME_SLICE : pProcess->iRemainingBurstTime;
  runProcess(pProcess, iBurstTime, bSimulatePageFaults);
}

/*
 * Function returning the time difference in milliseconds between the two time stamps, with start being the earlier time, and end being the later time.
 */
long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
  long int iSeconds = end.tv_sec - start.tv_sec;
  long int iUSeconds = end.tv_usec - start.tv_usec;
  long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
  return mtime;
}

void mapFrame(Process * pProcess, FrameEntry * pFrame) {
  pthread_mutex_lock(&pFrame->m);
  pFrame->iPID = pProcess->iPID;
  pFrame->iPage = pProcess->oLastRequested.iPage;
  pFrame->iAccessed = 1;
  pFrame->iDirty = 0;
  pProcess->apPageTable[pProcess->oLastRequested.iPage] = pFrame;
  pthread_mutex_unlock(&pFrame->m);
}

void mapDummyFrame(Process * pProcess) {
  FrameEntry * pFrame = pProcess->apPageTable[pProcess->oLastRequested.iPage] = malloc(sizeof(FrameEntry));
  *pFrame = (FrameEntry) PAGE_TABLE_ENTRY_INITIALIZER;
  pFrame->iPID = pProcess->iPID;
  pFrame->iPage = 1;
  pFrame->iAccessed = 1;
  pFrame->iDirty = 0;
}


void reclaimFrame(FrameEntry * pFrame) {
  pthread_mutex_lock(&pFrame->m);
  pFrame->iPID = -1;
  pFrame->iPage = -1;
  pFrame->iAccessed = 0;
  pFrame->iDirty = 0;
  pthread_mutex_unlock(&pFrame->m);
}
