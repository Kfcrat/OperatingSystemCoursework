#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include "coursework.h"
#include "linkedlist.h"

sem_t GenS;
sem_t SimS;
sem_t TermDaeS;

int termCounter=0;
int SimuCounter =0;
int GenCounter = 0;
int i = 0;

Process * processGen;
Process * processSim;
Process * processTerm;

Element * tmpElement;
Element * nextElement;

LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
LinkedList oTerminatedQueue = LINKED_LIST_INITIALIZER;
LinkedList hashTable[SIZE_OF_PROCESS_TABLE];

void * processGenerator ()
{
  while (GenCounter< NUMBER_OF_PROCESSES)
  {
    sem_wait(&GenS);//sleep when reach 0
    processGen = generateProcess(GenCounter); //generator process
    GenCounter++; //increment ID

    printf("ADMITTED: [PID = %d, Hash =  %d, BurstTime =  %d, RemainingBurstTime = %d, Locality= %d, Width = %d ]\n", processGen->iPID, processGen->iHash, processGen ->iBurstTime,processGen ->iRemainingBurstTime, processGen ->iLocality, processGen ->iWidth);

    addLast (processGen, &oReadyQueue);  //add process to Readyqueue
    addLast (processGen, &hashTable[processGen->iHash]); //add process to hash table

    sem_post(&SimS);  //wake up simulaotr
  }
}

void * processSimulator()
{

 sem_wait(&SimS);

  while (SimuCounter < NUMBER_OF_PROCESSES){

  tmpElement =  getHead (oReadyQueue);  //save head of ready queue to tmpElement

  while (tmpElement != NULL)
  {
    processSim = tmpElement->pData;     //get from head or from stored nextElement from last iteration
    nextElement = tmpElement -> pNext;  //get the next element and store in nextElement
    runPreemptiveProcess(processSim, false);
    printf("SIMULATING : [ PID = %d, BurstTime =  %d, RemainingBurstTime = %d ]\n", processSim->iPID, processSim ->iBurstTime, processSim ->iRemainingBurstTime);

    if (processSim-> iStatus == READY)
    {
      addLast (processSim, &oReadyQueue);  //re-add to ready queue
      removeData (processSim,&oReadyQueue);
      printf("READY : [PID = %d, BurstTime =  %d, RemainingBurstTime = %d, Locality= %d, Width = %d ]\n",processSim->iPID, processSim ->iBurstTime,processSim ->iRemainingBurstTime, processSim ->iLocality, processSim ->iWidth);
    }
    else
    {
      addLast (processSim, &oTerminatedQueue);//add to terminate queue
      removeData (processSim,&oReadyQueue); //remove from ready queue

      SimuCounter++;       //increment ID
      sem_post(&TermDaeS);  //wake up terminator
    }

    tmpElement = nextElement;   //store nextElement in tmpElement
  }

}
}

 void * processTerminator ()
{
  while (termCounter< NUMBER_OF_PROCESSES)
  {
    sem_wait(&TermDaeS);

    processTerm = removeFirst(&oTerminatedQueue);
    printf("TERMINATED : [PID = %d, RemainingBurstTime = %d ]\n" , processTerm->iPID, processTerm ->iRemainingBurstTime );

    termCounter++;

    removeData (processTerm, &hashTable[processTerm-> iHash]);
    long int TurnAroundTime= getDifferenceInMilliSeconds( processTerm ->oFirstTimeRunning, processTerm->oLastTimeRunning);
    long int responseTime = getDifferenceInMilliSeconds( processTerm ->oTimeCreated, processTerm->oFirstTimeRunning);
    printf("CLEARED : [ PID = %d, ResponseTime = %d , TurnAroundTime = %d ]\n", processTerm->iPID, responseTime, TurnAroundTime);

    free (processTerm);
    sem_post(&GenS);
  }
}

int main ()
{
 pthread_t GenThread;
 pthread_t SimThread;
 pthread_t TermiThread;

 sem_init (&GenS, 0, MAX_CONCURRENT_PROCESSES );
 sem_init (&SimS, 0 , 0);
 sem_init (&TermDaeS , 0 , 0);

 while(i < SIZE_OF_PROCESS_TABLE)
    {
      LinkedList HashList = LINKED_LIST_INITIALIZER;
      hashTable[i] = HashList;
      i++;
    }

 pthread_create (&GenThread, NULL, processGenerator, NULL);
 pthread_create (&SimThread, NULL, processSimulator, NULL);
 pthread_create (&TermiThread, NULL, processTerminator, NULL);

 pthread_join (GenThread, NULL);
 pthread_join (SimThread,NULL);
 pthread_join (TermiThread,NULL);

 return 0;
}

