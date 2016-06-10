/*
 * PCB.h
 *
 *  This is a class which stores all the necessary information regarding the
 *  job.
 */

class PCB {

public:
  int JobID;
  int PC;
  int TraceFlag;
  int programLength;
  unsigned long Registers[17];
  int PageTablePoint;

  //CLOCKS and TIMES:
  int CPUEnterTime;
  int TimeTaken;
  int TimeOfCompletionOfIO;
  int IOTimeStamp;
  int ClockAtLoadTime;
  int ClockAtTerminationTime;
  int ClockAtEnterBlockedQueue;
  int PageFaultTime;

  // Disk read write offsets
  int numberOfDATAAvailable;
  int numberOfWriteAvailable;
  int numberOfDATAAvailable2;
  int numberOfWriteAvailable2;

  // Disk read and write addresses
  int DiskPageReadAddress;
  int DiskPageWriteAddress;

  // offset in disk page currently r/w from:
  int DiskPageReadAddressOffset;
  int DiskPageWriteAddressOffset;

  // DISK PAGE CURRENTLY R/W FROM:
  int DiskPageRead;
  int DiskPageWrite;

  //Page indices
  unsigned short PagesOnDisk[256];
  unsigned short DiskPagesForRead[256];
  unsigned short DiskPagesForWrite[256];

  //Number of pages:
  int NumberOfPagesOnDisk;
  int NumberOfPagesForProgram;
  int NumberOfPagesForRead;
  int NumberOfPagesForWrite;

  //Used to implement paging
  int LRUCount;

  //CPU turn count(n)
  int n;

  //QueueIndex;
  int QueueIndex;

  //CPU Shots
  int CPUShots;

  int timeSpentIn4thSubQueue;

};
