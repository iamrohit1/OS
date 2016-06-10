/*
 * MEMORY_MANAGER.h
 *
 *  Gloabal Variables:
 *  occupiedPageFlag: keeps track of pages on memory that are used up
 *  occupiedPAgeTable: keeps track of pagetables available to be used
 *  PageTable: Page tables array. 5 bits are for page#,valid,reference dirty
 *  			and count for LRU respectively.
 *  ReadyQueueJobIDs: List of all the jobs in ready queue, for easy access.
 *
 *  Memory manager contains set of methods used by SYSTEM to manage the memory effective
 *  -ly.
 *
 *  Methods:
 *  initializeReadyQueueJobIDs: initializes the jobIDs to -1.
 *  pushInReadyQueueJobIDS:
 *  popInReadyQueueJobIDS: abuve and this methods are basically getter and setter
 *  						functions for reafyQueueJobIDs array.
 *  copyPageDiskMemory:copies page from disk to memory
 *  returnEmptyPage: returns empty page on memory
 *  returnEmptyPageTable: returns point to an empty pagetable
 *  checkIfJobOnMemory: Ck=hecks if the job has been already loaded.
 *  numberOfEmptyPagesOnMemory: Returns empty slots on memory.
 *  calculatePhysicalLocation: Perhaps the most important function in memory, this calculates
 *  	the physical address from the given virtual address by indexing in pagetable.
 *  returnReplacePageIndex: Gives the index of the page to be replaced by implementing LRU.
 *  replacePage: Replaces the page on memory.
 */

unsigned long MEM[0x100];

bool occupiedPageFlag[16];
bool occupiedPageTable[16];
unsigned short PageTable[16][16][5]; // page table job/content/ 0-page no. 1 - Invalid bit,2-reference bit,3-dirty bit,4-used count for LRU
int readyQueueJobIDs[16];

void initializeReadyQueueJobIDs() {
  for (int i = 0; i < 16; i++){
    readyQueueJobIDs[i] = -1;
    occupiedPageTable[i] = false;
    occupiedPageFlag[i] = false;
  }

}

void pushInReadyQueueJobIDS(int jobID) {
  for (int i = 0; i < 16; i++){

    if (readyQueueJobIDs[i] == -1){
      readyQueueJobIDs[i] = jobID;
      return;
    }

  }
}

void popInReadyQueueJobIDS(int jobID) {
  for (int i = 0; i < 16; i++){
    if (readyQueueJobIDs[i] == jobID){
      readyQueueJobIDs[i] = -1;
      break;
    }
  }
}

void copyPageDiskMemory(int DiskPage, int MemoryPage, bool flag) {
  int diskAddress = DiskPage * 0x10;
  int memoryAddress = MemoryPage * 0x10;
  if (flag){
    for (int i = 0; i < 16; i++)
      MEM[memoryAddress++] = DISK[diskAddress++];
  }else{
    for (int i = 0; i < 16; i++)
      DISK[diskAddress++] = MEM[memoryAddress++];
  }
}

int returnEmptyPage(int from) {

  for (int i = 0; i < 16; i++){

    if (occupiedPageFlag[from] == false){
      occupiedPageFlag[from] = true;
      return from;
    }
    from++;
    if (from > 15){
      from = 0;
    }
  }

  return -1;
}

int returnEmptyPageTable(int from) {

  for (int i = 0; i < 16; i++){

    if (occupiedPageTable[from] == false){
      occupiedPageTable[from] = true;
      return from;
    }
    from++;
    if (from > 15){
      from = 0;
    }
  }

  return -1;
}

bool checkIfJobOnMemory(PCB diskJob) {

  //int limit = readyQueue.size();

  for (int i = 0; i < 16; i++){

    if (diskJob.JobID == readyQueueJobIDs[i]){
      return true;
    }

  }
  return false;
}

int numberOfEmptyPagesOnMemory() {
  int count = 0;
  for (int i = 0; i < 16; i++){
    if (occupiedPageFlag[i] == false)
      count++;
  }
  return count;
}

int calculatePhysicalLocation(unsigned short pageTable[16][5], int location,
  int length) {

  int pageTableIndex = location / 0x10;
  location = location % 0x10;

  int limit = length / 16;
  if (length % 16 != 0){
    limit++;
  }

  location = location + (pageTable[pageTableIndex][0] * 0x10);

  return location;
}

int returnReplacePageIndex(int pageTableIndex) {

  int sum;
  int index = -1;
  int newsum;
  for (int i = 0; i < 16; i++){

    if (PageTable[pageTableIndex][i][1] == 1){
      if (index == -1){
        index = i;
        sum = PageTable[pageTableIndex][0][2] + PageTable[pageTableIndex][0][3];
      }

      newsum = PageTable[pageTableIndex][i][2]
        + PageTable[pageTableIndex][i][3];
      if (sum > newsum){
        sum = PageTable[pageTableIndex][i][2] + PageTable[pageTableIndex][i][3];
        index = i;
      }else if (sum == newsum){
        // LRU implementation
        if (PageTable[pageTableIndex][i][4]
          < PageTable[pageTableIndex][index][4]){
          index = i;
        }
      }
    }
  }

  return index;
}

void replacePage(int pageTableIndex, int index) {
  CLOCK += 5;
  currentJobPCB.PageFaultTime += 5;
  totalPageTime += 5;
  pageFaultCount++;
  int oldIndex = returnReplacePageIndex(pageTableIndex);

  if (PageTable[pageTableIndex][oldIndex][3] == 1){
    //Copy pages from Memory to
    copyPageDiskMemory(currentJobPCB.PagesOnDisk[oldIndex],
      PageTable[pageTableIndex][oldIndex][0], false);
  }

  // Change valid bits
  PageTable[pageTableIndex][oldIndex][1] = 0;
  PageTable[pageTableIndex][index][1] = 1;

  PageTable[pageTableIndex][index][0] = PageTable[pageTableIndex][oldIndex][0]; // Memory Page Swap

  // Copy words from Disk to Memory
  copyPageDiskMemory(currentJobPCB.PagesOnDisk[index],
    PageTable[pageTableIndex][index][0], true);

  PageTable[pageTableIndex][index][2] = 1;
  PageTable[pageTableIndex][index][3] = 0;
  PageTable[pageTableIndex][index][4] = ++currentJobPCB.LRUCount;

  //check and set reference bits to 0
  for (int i = 0; i < currentJobPCB.NumberOfPagesForProgram; i++){
    if (PageTable[pageTableIndex][i][2] == 0)
      break;
    if (i == 16){
      for (int i = 0; i < 16; i++)
        PageTable[pageTableIndex][i][2] = 0;
    }
  }

}

