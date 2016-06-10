/*
 * DISK_MANAGER.h
 *
 *  Disk Manager handles all the functions of disk, Paging is implemented on disk
 *  same as MEMORY.
 *  Global Variables:
 *  diskJobs : An array of objects of PCB class which are used by jobs on disk
 *  usablePCB's: A boolean array to keep track of unused pcb's easily.
 *  jobCount: Gives the number of jobs loaded on to disk.
 *	DiskPageTable: an pagetable for disk
 *	DiskPageTable occupied :   keeps track of the unused page tables.
 *
 */

unsigned short DiskPageTable[0x100];
bool DiskPageTableOccupied[0x100];

PCB diskJobs[0x100];
bool usablePCBs[0x100];
int jobCount;

int numberOfPCBsUsed() {
  int count = 0;
  for (int i = 0; i < 100; i++)
    if (usablePCBs[i] == false){
      count++;
    }
  return count;
}

#include "MEMORY_MANAGER.h"
#include "SPOOLER.h"

bool checkLine(string line, bool space) {
  for (unsigned int i = 0; i < line.length(); i++){
    if (!(
    // Check for non hex digits
    ((int) line[i] >= 48 && (int) line[i] <= 57)
      || ((int) line[i] >= 65 && (int) line[i] <= 70)
      || ((int) line[i] >= 97 && (int) line[i] <= 102)

    )){

      if (!(space && ((int) line[i] == 32))){
        return true;
      }

    }
  }
  return false;
}

void SkipTillNextJob(ifstream &myFile, string &line, int &section) {
  section = 2;
  while (getline(myFile, line)){
    if (line.substr(line.length() - 1, line.length()) == "\r")
      line = line.substr(0, line.length() - 1);

    linesReadFromInputBatch++;
    if (line.substr(0, 5) == "**JOB"){
      break;
    }
  }

}

int calculatePhysicalLocationOnDisk(unsigned short pageTable[20],
  int location) {

  int pageTableIndex = location / 0x10;
  location = location % 0x10;

  location = location + (pageTable[pageTableIndex] * 0x10);

  return location;
}
int storeDATAOnDisk(unsigned short pages[20], int &pageUsedData,
  int &diskPageDataStore, int limit) {

  if (diskPageDataStore == 16){
    diskPageDataStore = 0;
    pageUsedData++;

    if (pageUsedData >= limit){
      return -1;
    }
  }

  int location = (pages[pageUsedData] * 0x10) + diskPageDataStore++;

  return location;
}

int numberOfEmptyPagesOnDisk() {
  int count = 0;
  for (int i = 0; i < 0x100; i++){
    if (DiskPageTableOccupied[i] == false)
      count++;
  }
  return count;
}

void initializeEmptyDiskAndPCBs() {

  for (int i = 0; i < 0x100; i++)
    DiskPageTableOccupied[i] = false;

  for (int i = 0; i < 100; i++)
    usablePCBs[i] = true;
}

int getEmptyPageOnDisk() {
  for (int i = 0; i < 0x100; i++){

    if (DiskPageTableOccupied[i] == false){
      DiskPageTableOccupied[i] = true;
      return i;
    }

  }

  return -1;
}

int getEmptyPCB() {
  for (int i = 0; i < 100; i++)
    if (usablePCBs[i] == true){
      usablePCBs[i] = false;
      return i;
    }
  return -1;
}

void DiskLoad() {

  int section = 2; // 0 - Job // 1 - Data // 2 - Fin
  int prevousSection = 2;

  int location; // variable to point on disk.
  int length;
  int IndexPCB;

  string file = FN[1];
  string line;
  ifstream myFile((file + "").c_str());
  unsigned long buffer[4];

  if (myFile.is_open()){

    for (int i = 0; i < linesReadFromInputBatch; i++){
      getline(myFile, line);
      line = line.substr(0, line.length() - 1);
    }

    int diskPageDataStore = 0;
    bool dataEmptyFlag;
    bool jobEmptyFlag = false;
    int pageUsedData = 0;

    while (getline(myFile, line)){
      if (line.substr(line.length() - 1, line.length()) == "\r")
        line = line.substr(0, line.length() - 1);

      linesReadFromInputBatch++;

      int numberOfPagesPD;
      int numberOfWords;
      int ProgramPages;

      if (line.substr(0, 2) == "**"){

        if (line.substr(2, 3) == "JOB"){
          section = 0;
          jobEmptyFlag = false;
          nextJobLabel: if (prevousSection != 2){
            //No Fin occured
            if (section == prevousSection){
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(104);
            }else
              currentJobPCB = diskJobs[IndexPCB];
            ERROR(105);

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

          }

          location = -1;
          length = 0;
          section = 0;
          jobCount++;

          IndexPCB = getEmptyPCB();
          diskJobs[IndexPCB].JobID = jobCount;

          diskJobs[IndexPCB].CPUEnterTime = -1; // Initializing to -1 for CPU to detect if job enters first time in CPU
          diskJobs[IndexPCB].ClockAtLoadTime = CLOCK;

          if (line.length() < 11){
            // Job Format error !!
            currentJobPCB = diskJobs[IndexPCB];
            ERROR(106);
            usablePCBs[IndexPCB] = true;

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }

          stringstream(line.substr(6, 2)) >> hex >> numberOfPagesPD;
          stringstream(line.substr(9, 2)) >> hex >> numberOfWords;
          numberOfPagesPD++;
          diskJobs[IndexPCB].numberOfWriteAvailable = numberOfWords * 4;
          diskJobs[IndexPCB].numberOfWriteAvailable2 = 0;

          if (numberOfWords > 0){
            diskJobs[IndexPCB].NumberOfPagesForWrite = (numberOfWords / 4);
            if (numberOfWords % 4 > 0)
              diskJobs[IndexPCB].NumberOfPagesForWrite++;
          }else{
            diskJobs[IndexPCB].NumberOfPagesForWrite = 0;
          }

          diskJobs[IndexPCB].NumberOfPagesOnDisk = numberOfPagesPD
            + diskJobs[IndexPCB].NumberOfPagesForWrite;

          if (numberOfEmptyPagesOnDisk()
            < numberOfPagesPD + diskJobs[IndexPCB].NumberOfPagesForWrite){
            jobCount--;
            linesReadFromInputBatch--;
            usablePCBs[IndexPCB] = true;
            break;
          }

          prevousSection = 0;
        }else if (line.substr(2, 4) == "DATA"){
          diskPageDataStore = 0;
          pageUsedData = 0;
          if (section != 0){
            //no Job Block
            if (section == prevousSection){
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(107);
            }else{
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(108);
            }

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }
          if (!jobEmptyFlag){
            currentJobPCB = diskJobs[IndexPCB];
            ERROR(109);

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }
          section = 1;
          prevousSection = section;
        }else if (line.substr(2, 3) == "FIN"){
          section = 2;
          if (prevousSection != 1){
            // No Data occured
            if (section == prevousSection){
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(110);
            }else{
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(111);
            }

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }

          if (!dataEmptyFlag){
            diskJobs[IndexPCB].numberOfDATAAvailable = 0;

          }
          int dataPagesUsed = diskJobs[IndexPCB].numberOfDATAAvailable / 16;
          if (diskJobs[IndexPCB].numberOfDATAAvailable % 16 > 0){
            dataPagesUsed++;
          }
          if (dataPagesUsed < diskJobs[IndexPCB].NumberOfPagesForRead){
            currentJobPCB = diskJobs[IndexPCB];
            if (diskJobs[IndexPCB].NumberOfPagesForRead - dataPagesUsed > 1)
              ERROR(152);
            //Free the unused pages given
            for (int i = dataPagesUsed;
              i < diskJobs[IndexPCB].NumberOfPagesForRead; i++){
              DiskPageTableOccupied[diskJobs[IndexPCB].DiskPagesForRead[i]] =
                false;
            }
            diskJobs[IndexPCB].NumberOfPagesForRead = dataPagesUsed;

          }
          ofstream myFS;
          myFS.open(FN[0], ios_base::app);
          myFS << "\n\n CLOCK(dec): " << CLOCK << "\t" << "JOBID(hex) : "
            << diskJobs[IndexPCB].JobID;
          myFS.close();
          prevousSection = section;

        }else{
          // Formnat error !
        }

      }else{

        switch (section) {
        case 0:
          if (line.substr(2, 1) == " "){
            if (location == -1){
              if (checkLine(line, true)){
                currentJobPCB = diskJobs[IndexPCB];
                ERROR(103);

                SpoolFromDisk(diskJobs[IndexPCB].JobID);

                // Skip till next job.
                SkipTillNextJob(myFile, line, section);
                prevousSection = 2;
                goto nextJobLabel;
              }
              jobEmptyFlag = true;
              // Get startLocation and length
              stringstream(line.substr(0, 2)) >> hex >> location;
              stringstream(line.substr(3, 2)) >> hex >> length;
              diskJobs[IndexPCB].programLength = length;

              ProgramPages = length / 16;
              int numLoaderLines = length / 4;
              if ((length % 16) > 0){
                ProgramPages++;
              }
              if ((length % 4) > 0){
                numLoaderLines++;
              }

              diskJobs[IndexPCB].NumberOfPagesForProgram = ProgramPages;

              diskJobs[IndexPCB].NumberOfPagesForRead =
                diskJobs[IndexPCB].NumberOfPagesOnDisk
                  - diskJobs[IndexPCB].NumberOfPagesForProgram
                  - diskJobs[IndexPCB].NumberOfPagesForWrite;

              if (((diskJobs[IndexPCB].NumberOfPagesOnDisk
                - diskJobs[IndexPCB].NumberOfPagesForWrite) * 16)
                - diskJobs[IndexPCB].programLength > 0){
                //diskJobs[IndexPCB].NumberOfPagesForRead++;
              }

              if ((ProgramPages
                > diskJobs[IndexPCB].NumberOfPagesOnDisk
                  - diskJobs[IndexPCB].NumberOfPagesForRead)
                || (diskJobs[IndexPCB].NumberOfPagesOnDisk
                  + diskJobs[IndexPCB].NumberOfPagesForWrite > 16)){
                currentJobPCB = diskJobs[IndexPCB];
                ERROR(112);

                SpoolFromDisk(diskJobs[IndexPCB].JobID);

                // Skip till next job.
                SkipTillNextJob(myFile, line, section);
                prevousSection = 2;
                goto nextJobLabel;
              }
              if (ProgramPages
                == diskJobs[IndexPCB].NumberOfPagesOnDisk
                  - diskJobs[IndexPCB].NumberOfPagesForWrite){
                dataEmptyFlag = true;
              }else{
                dataEmptyFlag = false;
              }

              // Assign pages to program
              for (int i = 0; i < diskJobs[IndexPCB].NumberOfPagesForProgram;
                i++){
                diskJobs[IndexPCB].PagesOnDisk[i] = getEmptyPageOnDisk();
              }
              //Assign pages to Read
              for (int i = 0; i < (diskJobs[IndexPCB].NumberOfPagesForRead);
                i++){
                diskJobs[IndexPCB].DiskPagesForRead[i] = getEmptyPageOnDisk();
              }

              diskJobs[IndexPCB].DiskPageReadAddressOffset = 0;

              //Assign pages to Write
              for (int i = 0; i < diskJobs[IndexPCB].NumberOfPagesForWrite;
                i++){
                diskJobs[IndexPCB].DiskPagesForWrite[i] = getEmptyPageOnDisk();
              }
              diskJobs[IndexPCB].DiskPageWriteAddressOffset = 0;

              int lineNo = 0;
              // Read data record
              for (int i = 0; i < length; i++){

                if (i % 4 == 0){

                  getline(myFile, line);
                  if (line.substr(line.length() - 1, line.length()) == "\r")
                    line = line.substr(0, line.length() - 1);

                  linesReadFromInputBatch++;
                  lineNo++;

                  if (checkLine(line, false)){
                    if (line.length() == 4){
                      currentJobPCB = diskJobs[IndexPCB];
                      ERROR(113);

                    }else{
                      currentJobPCB = diskJobs[IndexPCB];
                      ERROR(103);
                    }

                    SpoolFromDisk(diskJobs[IndexPCB].JobID);

                    // Skip till next job.
                    SkipTillNextJob(myFile, line, section);
                    prevousSection = 2;
                    goto nextJobLabel;
                  }

                  if ((lineNo == numLoaderLines && length % 4 != 0
                    && line.length() == 32)
                    || (lineNo != numLoaderLines && line.length() != 32)
                    || (lineNo == numLoaderLines
                      && line.length() != 8 * (length % 4)
                      && (line.length() != 32))){
                    currentJobPCB = diskJobs[IndexPCB];
                    ERROR(103);

                    SpoolFromDisk(diskJobs[IndexPCB].JobID);

                    // Skip till next job.
                    SkipTillNextJob(myFile, line, section);
                    prevousSection = 2;
                    goto nextJobLabel;
                  }
                }
                stringstream(line.substr((i % 4) * 8, 8)) >> hex
                  >> buffer[i % 4];

                DISK[calculatePhysicalLocationOnDisk(
                  diskJobs[IndexPCB].PagesOnDisk, location)] = buffer[i % 4];
                location++;

              }

            }else{
              if (checkLine(line, true)){
                currentJobPCB = diskJobs[IndexPCB];
                ERROR(103);
                SpoolFromDisk(diskJobs[IndexPCB].JobID);

                // Skip till next job.
                SkipTillNextJob(myFile, line, section);
                prevousSection = 2;
                goto nextJobLabel;
              }
              // Get PC and TraceFlag
              stringstream(line.substr(0, 2)) >> hex >> diskJobs[IndexPCB].PC;
              stringstream(line.substr(3, 1)) >> hex
                >> diskJobs[IndexPCB].TraceFlag;

              // if traceflag > 1 Warning!! and Traceflag = 1;
              if (diskJobs[IndexPCB].TraceFlag != 0
                && diskJobs[IndexPCB].TraceFlag != 1){
                currentJobPCB = diskJobs[IndexPCB];
                ERROR(551);
                diskJobs[IndexPCB].TraceFlag = 0;
              }
            }

          }else{

            // Program block too long ERROR !!
            // Skip to next job !!
            currentJobPCB = diskJobs[IndexPCB];
            ERROR(114);

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }
          break;

        case 1:

          if (checkLine(line, true)){
            currentJobPCB = diskJobs[IndexPCB];
            ERROR(103);

            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }

          if (diskJobs[IndexPCB].NumberOfPagesForRead == 0){
            currentJobPCB = diskJobs[IndexPCB];
            ERROR(115);
            SpoolFromDisk(diskJobs[IndexPCB].JobID);

            // Skip till next job.
            SkipTillNextJob(myFile, line, section);
            prevousSection = 2;
            goto nextJobLabel;
          }

          dataEmptyFlag = true;
          for (int i = 0; i < 4; i++){
            stringstream(line.substr((i % 4) * 8, 8)) >> hex >> buffer[i % 4];

            int adr = storeDATAOnDisk(diskJobs[IndexPCB].DiskPagesForRead,
              pageUsedData, diskPageDataStore,
              diskJobs[IndexPCB].NumberOfPagesForRead);
            if (adr == -1){
              currentJobPCB = diskJobs[IndexPCB];
              ERROR(116);

              SpoolFromDisk(diskJobs[IndexPCB].JobID);
              diskPageDataStore = 0;
              pageUsedData = 0;
              // Skip till next job.
              SkipTillNextJob(myFile, line, section);
              prevousSection = 2;
              goto nextJobLabel;
            }
            DISK[adr] = buffer[i % 4];

            diskJobs[IndexPCB].numberOfDATAAvailable++;

          }

          break;
        case 2:
          currentJobPCB = diskJobs[IndexPCB];
          ERROR(117);
          SkipTillNextJob(myFile, line, section);
          prevousSection = 2;
          goto nextJobLabel;

          break;
        default:
          //Should not reach here
          break;
        }
      }

    }

  }else{
    // File read error
    ERROR(100);
    return;
  }

}
