/*
 * MEMORY.h
 *
 *
 *	Description:
 *		This module controls the memory read and write.
 *	Gloabal variables:
 *		MEM: in an unsigned long(32bit) array which acts as our memory.
 *		PageTableIndex : points to the page table that current job uses.
 *
 *	MEMORY function: Reads or Writes and Dumps the memory as specified in specifications.
 *
 */

#include<stdio.h>

using namespace std;

int PageTableIndex;

void MEMORY(string X, int Y, unsigned long &Z) {

  ofstream myFS;

  PageTableIndex = currentJobPCB.PageTablePoint;
  // Replace a page in memory if the requested page is not on memory
  // i.e. its valid bit is 0
  if (PageTable[PageTableIndex][Y / 16][1] == 0){
    replacePage(PageTableIndex, (Y / 16));
  }

  // Taking cases to switch on
  int z;
  if (X == "READ"){
    z = 1;
  }else if (X == "WRIT"){
    z = 2;
  }else if (X == "DUMP"){
    z = 3;
  }

  // Switching through read,write and dump
  switch (z) {

  case 1: // Read from memory to MemoryBufferRegester
    PageTable[PageTableIndex][Y / 16][2] = 1; //Reference bit
    Y = calculatePhysicalLocation(PageTable[PageTableIndex], Y,
      currentJobPCB.programLength);
    Z = MEM[Y];

    break;

  case 2: // Write from MemoryBufferRegister to memory
    PageTable[PageTableIndex][Y / 16][2] = 1; //Reference bit
    PageTable[PageTableIndex][Y / 16][3] = 1; //Dirty bit
    Y = calculatePhysicalLocation(PageTable[PageTableIndex], Y,
      currentJobPCB.programLength);

    MEM[Y] = Z;
    break;

  case 3: // Dump

    myFS.open(FN[0], ios_base::app);
    myFS << "\n\n CLOCK(dec):\t" << CLOCK << "\tJOBID(hex): "
      << currentJobPCB.JobID << "\n";
    for (int i = 0; i < currentJobPCB.programLength; i = i + 8){
      myFS << "\t" << setfill('0') << setw(4) << hex << i << "\t";
      for (int j = 0; j < 8 && i + j < currentJobPCB.programLength; j++){
        myFS << setfill('0') << setw(8) << hex
          << MEM[calculatePhysicalLocation(PageTable[PageTableIndex], (i + j),
            currentJobPCB.programLength)] << " ";
      }
      myFS << "\n";
    }
    myFS.close();
    break;

  default:
    // Does not reach here.
    break;
  }

}

