/*
 * LOADER.h
 *
 *  The only purpose of loader is to load the job program pages from disk to
 *  MEMORy, however not all the pages are loaded, only one-third of the pagers or
 *  two pages are loaded, which ever is the greatest.
 *  The control flow of the loader is straight forward, i.e. look for empty pages
 *  on memory, if exixt then load on to memory.
 *
 */
void LOADER() {
  int limit = numberOfPCBsUsed();
  for (int i = 0; i < limit; i++){
    if (usablePCBs[i] == true){ // Skipping non used PCBs
      limit++;
    }else{

      int pages = diskJobs[i].NumberOfPagesForProgram / 3;
      if (pages < 2)
        pages = 2;

      // If pages on memory can fit the current PCB's Job
      if ((pages) <= numberOfEmptyPagesOnMemory()){

        // If the Job is not on the memory already
        if (!checkIfJobOnMemory(diskJobs[i])){

          //Get an unused pageTable and assign it in PCB
          int pageTableIndex = returnEmptyPageTable(0);
          diskJobs[i].PageTablePoint = pageTableIndex;
          //get empty pages and copy pages from disk to memory
          for (int j = 0; j < pages; j++){

            PageTable[pageTableIndex][j][0] = returnEmptyPage(0); // Get empty memory page
            PageTable[pageTableIndex][j][1] = 1; //Valid Bit
            PageTable[pageTableIndex][j][2] = 0; //Reference Bit
            PageTable[pageTableIndex][j][3] = 0; //Dirty Bit
            PageTable[pageTableIndex][j][4] = 0; //LRU count initializations
            // Load into memory
            copyPageDiskMemory(diskJobs[i].PagesOnDisk[j],
              PageTable[pageTableIndex][j][0], true);

          }

          for (int k = pages; k < diskJobs[i].NumberOfPagesForProgram; k++){

            PageTable[pageTableIndex][k][1] = 0;						//Valid Bit
            PageTable[pageTableIndex][k][2] = 0;						//Reference Bit
            PageTable[pageTableIndex][k][3] = 0;						//Dirty Bit
            PageTable[pageTableIndex][k][4] = 0;		//LRU count initializations

          }

          pushInReadyQueueJobIDS(diskJobs[i].JobID);
          subQueue[0].push(diskJobs[i]);
        }
      }
    }

  }
}
