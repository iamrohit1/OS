//============================================================================
// Name        : Rohit Nutalapati
// Course No.  : 5323
// Assignment  :STEP III (OSII Project)
// Created on  : MAR 6, 2016
//
// Description : SYSTEM.cpp
//
//	Global variables:
//
// 	CLOCK,CHANNEL_CLOCK,CPU_CLOCK,TIME_STAMP : Used to store clock values
//  in the program
//	*FN[2]  				 : Character pointer used to point to file
//						   names location. (Input file and output
//						   file)
//	linesReadFromInputBatch : keeps track of lines read from the input
//                          batch
//	flag_reguralInterval	: keeps track of regular intervals on which System
//                        status has to be printed
//	Function:
//	System is the main block(cpp file) consisting of 6 components CPU,LOADER,
//MEMORY,PCB,DISK_MANAGER and ERROR HANDLER. All of the components are defined
//in a seperate header files and importedfor modularity. Functions that SYSTEM
//performs are:
//	1) Initialize variables.
//	2) Invoke the DiskLoad to load programs to disk, get the StartAddress and
//     Trace flag..
//	3) Invoke LOADER to load programs on to MEMORY.
//	4) Start execution of jobs in ready list.
//	   Aditionally it also handles :
//	5) Input/Output from/to user (Which is invoked by the CPU).
//
//============================================================================

#include <queue>
using namespace std;

int linesReadFromInputBatch, flag_reguralInterval;
int CLOCK, CHANNEL_CLOCK, CPU_CLOCK, TIME_STAMP;
int n = 5, q = 50;
int nValues[3] = { 3, 4, 5 };
int qValues[4] = { 35, 40, 45, 50 };

char const *FN[2];

unsigned int DISK[0x1000];
//SYSTEM METRICS
int totalRunTime;
int totalIOTime;
int totalExecutionTime;
int totalTurnAroundTime;
int totalPageTime;
int totalCPUidealTime;
int totalErrorJobTime;
int numberOfNormalJobs;
int numberOfAbnormalJobs;
int totalSuspectInfiniteJobtime;
queue<int> InfiniteJobIDs;
int pageFaultCount;
int DiskHoleCount;
int DiskCount;
int DiskHoleSampleCount;
//QUEUE METRICS
long int subqueuesize[4];
int subqueuesizeSampleCount;
int fiveHCount, flag = 0;
int countMLFBQinterval;
int migrationsInSubqueueCount;
int maxJobsInSubqueues[4];

#include "PCB.h"
PCB currentJobPCB;
queue<PCB> blockedQueue;
//queue<PCB> readyQueue;
queue<PCB> tempQueue;
//4 subQueues as MLFBQ
queue<PCB> subQueue[4];

#include "ERROR_HANDLER.h"
#include "DISK_MANAGER.h"
#include "LOADER.h"
#include "MEMORY.h"
#include "CPU.h"

using namespace std;
// To take input from user, Called by CPU.

void resetMetrics() {

}

void systemI(int jobEnterTime, int PC) {

  currentJobPCB.IOTimeStamp = CLOCK;

  if (currentJobPCB.numberOfDATAAvailable <= 0){
    ERROR(501);
    return;
  }

  // Buffer for block transfer
  unsigned long buffer[4];

  buffer[0] = DISK[calculatePhysicalLocationOnDisk(
    currentJobPCB.DiskPagesForRead, currentJobPCB.DiskPageReadAddress++)]
    % 0x100000000;
  buffer[1] = DISK[calculatePhysicalLocationOnDisk(
    currentJobPCB.DiskPagesForRead, currentJobPCB.DiskPageReadAddress++)]
    % 0x100000000;
  buffer[2] = DISK[calculatePhysicalLocationOnDisk(
    currentJobPCB.DiskPagesForRead, currentJobPCB.DiskPageReadAddress++)]
    % 0x100000000;
  buffer[3] = DISK[calculatePhysicalLocationOnDisk(
    currentJobPCB.DiskPagesForRead, currentJobPCB.DiskPageReadAddress++)]
    % 0x100000000;

  currentJobPCB.numberOfDATAAvailable = currentJobPCB.numberOfDATAAvailable - 4;

  // Block transfer simulation.
  MBR[0] = buffer[0];
  MBR[1] = buffer[1];
  MBR[2] = buffer[2];
  MBR[3] = buffer[3];
  currentJobPCB.TimeOfCompletionOfIO += 8;

  //if(readyQueue.size() != 0)
  JobSuspendIO(currentJobPCB, (CLOCK - jobEnterTime + 8), PC);

  return;
}

// To show output to user, called by CPU.
void systemO(int jobEnterTime, int PC) {

  currentJobPCB.IOTimeStamp = CLOCK;

  if (currentJobPCB.numberOfWriteAvailable <= 0){
    ERROR(502);
    return;
  }

  currentJobPCB.numberOfWriteAvailable2 += 4;

  DISK[calculatePhysicalLocationOnDisk(currentJobPCB.DiskPagesForWrite,
    currentJobPCB.DiskPageWriteAddress++)] = MBR[0];
  DISK[calculatePhysicalLocationOnDisk(currentJobPCB.DiskPagesForWrite,
    currentJobPCB.DiskPageWriteAddress++)] = MBR[1];
  DISK[calculatePhysicalLocationOnDisk(currentJobPCB.DiskPagesForWrite,
    currentJobPCB.DiskPageWriteAddress++)] = MBR[2];
  DISK[calculatePhysicalLocationOnDisk(currentJobPCB.DiskPagesForWrite,
    currentJobPCB.DiskPageWriteAddress++)] = MBR[3];

  currentJobPCB.numberOfWriteAvailable = currentJobPCB.numberOfWriteAvailable
    - 4;
  currentJobPCB.TimeOfCompletionOfIO += 8;

  //if(readyQueue.size() != 0)
  JobSuspendIO(currentJobPCB, (CLOCK - jobEnterTime + 8), PC);

  return;
}
void programReset() {
  flag_reguralInterval = 0;
  countMLFBQinterval = 0;
  DiskCount = 0;
  fiveHCount = 0;
  totalRunTime = 0;
  numberOfNormalJobs = 0;
  numberOfAbnormalJobs = 0;
  totalIOTime = 0;
  totalRunTime = 0;
  totalExecutionTime = 0;
  totalTurnAroundTime = 0;
  totalPageTime = 0;
  totalErrorJobTime = 0;
  totalSuspectInfiniteJobtime = 0;
  while (!InfiniteJobIDs.empty()){
    InfiniteJobIDs.pop();
  }
  for (int i = 0; i < 0x1000; i++)
    DISK[i] = 0;
  for (int i = 0; i < 16; i++)
    MEM[i] = 0;
  pageFaultCount = 0;
  DiskHoleCount = 0;
  flag = 0;
  DiskHoleSampleCount = 0;
  for (int i = 0; i < 4; i++){
    subqueuesize[i] = 0;
    maxJobsInSubqueues[i] = 0;
  }
  CLOCK = 0;
  subqueuesizeSampleCount = 0;
}

int main(int x, char *File[]) {
  // Initializing clocks.
  CLOCK = 0;
  CHANNEL_CLOCK = 0;

  FN[0] = "execution_profile"; //File[2]; // output file name
  FN[1] = File[1]; // input file name
  if (FN[0] == NULL){
    FN[1] = "output.txt";
  }

  numberOfNormalJobs = 0;
  numberOfAbnormalJobs = 0;
  jobCount = -1;
  ofstream myFSM;
  string s = "MATRIX";
  myFSM.open(s.c_str(), ios_base::app);
  myFSM << "\n\t  \t(dec)\t\t(dec)\t\t(dec)\t\t(dec)";
  myFSM << "\n\tq:\t35\t\t40\t\t45\t\t50\nn:\n";
  for (int N = 0; N < 3; N++){
    n = nValues[N];
    myFSM << n << "(dec)\t\t";
    for (int Q = 0; Q < 4; Q++){
      programReset();
      q = qValues[Q];
      ofstream myFS4;
      string s = "MLFBQ";
      myFS4.open(s.c_str());
      myFS4 << "\n";
      myFS4.close();
      jobCount = -1;
      initializeEmptyDiskAndPCBs();
      initializeReadyQueueJobIDs();
      linesReadFromInputBatch = 0;

      DiskLoad();
      LOADER();

      ofstream myFS;
      myFS.open(FN[0]);
      myFS.close();

      // CPU Execution
      while (!subQueue[0].empty() || !subQueue[1].empty()
        || !subQueue[2].empty() || !subQueue[3].empty() || !blockedQueue.empty()){

        //Calculate maximum subqueue metrics
        for (int i = 0; i < 4; i++){
          if (maxJobsInSubqueues[i] < subQueue[i].size()){
            maxJobsInSubqueues[i] = subQueue[i].size();
          }
        }
        //Fetching job from queues
        if (!subQueue[0].empty()){
          currentJobPCB = subQueue[0].front();
          subQueue[0].pop();
        }else if (!subQueue[1].empty()){
          currentJobPCB = subQueue[1].front();
          subQueue[1].pop();
        }else if (!subQueue[2].empty()){
          currentJobPCB = subQueue[2].front();
          subQueue[2].pop();
        }else if (!subQueue[3].empty()){
          currentJobPCB = subQueue[3].front();
          subQueue[3].pop();
        }else{
          //Handling idle CPU time
          currentJobPCB = blockedQueue.front();
          blockedQueue.pop();
          if (blockedQueue.size() == 0){
            totalCPUidealTime += (8
              - (CLOCK - currentJobPCB.ClockAtEnterBlockedQueue));
            CLOCK += (8 - (CLOCK - currentJobPCB.ClockAtEnterBlockedQueue));
          }else{

            while (blockedQueue.size() != 1){
              tempQueue.push(blockedQueue.front());
              blockedQueue.pop();
            }
            if ((blockedQueue.front().ClockAtEnterBlockedQueue
              - currentJobPCB.ClockAtEnterBlockedQueue) > 0
              && (blockedQueue.front().ClockAtEnterBlockedQueue
                - currentJobPCB.ClockAtEnterBlockedQueue) < 8){
              totalCPUidealTime += (8
                - (blockedQueue.front().ClockAtEnterBlockedQueue
                  - currentJobPCB.ClockAtEnterBlockedQueue));
              CLOCK += (8
                - (blockedQueue.front().ClockAtEnterBlockedQueue
                  - currentJobPCB.ClockAtEnterBlockedQueue));
            }
            tempQueue.push(blockedQueue.front());
            blockedQueue.pop();
            while (!tempQueue.empty()){
              blockedQueue.push(tempQueue.front());
              tempQueue.pop();
            }
          }

        }
        if (currentJobPCB.CPUEnterTime != -1){
          // Load regersters in CPU.
          loadCPURegisters(currentJobPCB);
        }else{
          // Initialization while reusing the PCB first time
          currentJobPCB.CPUEnterTime = CLOCK;
          currentJobPCB.DiskPageRead = 0;
          currentJobPCB.DiskPageWrite = 0;
          currentJobPCB.DiskPageReadAddressOffset = 0;
          currentJobPCB.DiskPageWriteAddressOffset = 0;
          currentJobPCB.DiskPageReadAddress = 0;
          currentJobPCB.DiskPageWriteAddress = 0;
          currentJobPCB.TimeOfCompletionOfIO = 0;
          currentJobPCB.PageFaultTime = 0;
          currentJobPCB.LRUCount = 0;
          currentJobPCB.numberOfWriteAvailable2 = 0;
          //Start trace if the job has trace enabled
          if (currentJobPCB.TraceFlag == 1){
            string s = numberToString(currentJobPCB.JobID);
            myFS.open(s.c_str(), ios_base::app);
            myFS << " PC(hex)\tINST(hex)\tA(hex)\t\tA(hex)\t\tEA(hex)\n";
            myFS << " \t\t\t\t(before)\t(after)\t\t(after)\n";
            myFS.close();
          }
        }
        exit_code = 0;
        //Execute Job
        CPU(currentJobPCB.PC, currentJobPCB.TraceFlag);

      }
      // Write exit type into output file.
      myFS.open(FN[0], ios_base::app);

      // Calculate Metrics and print.
      CPU_CLOCK = CLOCK - CHANNEL_CLOCK;
      myFS << "\n\n Mean user job runtime(dec):\t\t\t\t\t" << fixed
        << setprecision(2)
        << (double) totalRunTime
          / (double) (numberOfNormalJobs + numberOfAbnormalJobs);
      myFS << "\n Mean user job I/O time(dec):\t\t\t\t\t" << setprecision(2)
        << (double) totalIOTime
          / (double) (numberOfNormalJobs + numberOfAbnormalJobs);
      myFS << "\n Mean user job execution time(dec):\t\t\t\t" << setprecision(2)
        << (double) totalExecutionTime
          / (double) (numberOfNormalJobs + numberOfAbnormalJobs);
      myFS << "\n Mean user job time in SYSTEM(dec):\t\t\t\t" << setprecision(2)
        << (double) totalTurnAroundTime
          / (numberOfNormalJobs + numberOfAbnormalJobs);
      myFS << "\n Mean user job page-fault handling time(dec):\t\t\t"
        << setprecision(2)
        << (double) totalPageTime
          / (double) (numberOfNormalJobs + numberOfAbnormalJobs);
      myFS << "\n CPU ideal time(dec):\t\t\t\t\t\t" << totalCPUidealTime;
      myFS << "\n Time lost due to abnormally terminated jobs(dec):\t\t"
        << totalErrorJobTime;
      myFS << "\n Number of jobs terminated normally(dec):\t\t\t"
        << numberOfNormalJobs;
      myFS << "\n Number of jobs terminated abnormally(dec):\t\t\t"
        << numberOfAbnormalJobs;
      myFS << "\n Time lost due to suspect infinite job(dec):\t\t\t"
        << totalSuspectInfiniteJobtime;
      if (!InfiniteJobIDs.empty()){
        myFS << "\n Infinite job ID's(hex):\n";
        int lineCount = 10;
        while (!InfiniteJobIDs.empty()){
          //InfiniteJobIDs.pop();
          myFS << "\t" << hex << (int) InfiniteJobIDs.front();
          if (lineCount-- == 0){
            myFS << "\n";
            lineCount++;
          }
          InfiniteJobIDs.pop();
        }
      }
      myFS << "\n Total number of page-faults encountered(dec):\t" << dec
        << pageFaultCount;
      myFS << "\n Avrage hole size on disk(dec):\t\t\t\t\t" << setprecision(2)
        << (double) DiskHoleCount / (double) DiskHoleSampleCount;
      myFS << "\n Avrage number of pages occupied on Disk(dec):\t"
        << setprecision(2) << (double) DiskCount / (double) DiskHoleSampleCount;
      for (int i = 0; i < 4; i++){
        myFS << "\n Average size of sub-queue(dec)" << i + 1 << ":\t\t\t\t"
          << setprecision(2)
          << (double) subqueuesize[i] / (double) subqueuesizeSampleCount;
      }
      for (int i = 0; i < 4; i++){
        myFS << "\n Maximum number of jobs in sub-queue(dec)" << i + 1
          << ":\t\t\t" << maxJobsInSubqueues[i];
      }
      myFS << "\n";
      myFS.close();
      myFSM << migrationsInSubqueueCount << "\t\t";
      migrationsInSubqueueCount = 0;
      programReset();
    }
    myFSM << "\n";
  }
  myFSM.close();

  //cout<<migrationsInSubqueueCount;
  return 0;
}
