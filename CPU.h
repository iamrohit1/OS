/*
 * CPU.h
 *	CPU executes the jobs, contributes in context swtching. all the
 *	jobs are as per the given instruction format.
 *  Descriptions:
 *	Global variables
 *		A : is is an array used as ACCUMILATOR regiester by the CPU.
 *		MBR : MBR is memory buffer regester, right now having only four
 *		 regesters.
 *		number1,2: long integers used while handling negetive numbers,
 *		since default values are all unsigned.
 *	CPU block Methods
 *		1)beforeExeTrace() : writes contents of PC, Instruction and A in
 *		 tracefile.
 * 					(Called before execution of each instruction)
 *		2)afterExeTrace(): Writes contents of EA, A in tracefile.
 * 					(Called after execution of each instruction)
 *		3)CPU(): Fetches each instruction and executes it.
 *		4)GetRealNumber(): this function converts the given number into
 *		signed number and hands it over to sysyem.(MSB is sign bit)
 *		5)GetSystemNumber():this function converts the unsigned number into
 *		system specified numnber.
 *		6)HaltPrint():Information to be printed and actions to be taken when
 *		haltoccures.
 *		7)regularIntervalPrint():Information to be printed and actions to be
 *		 taken at regular intervals
 *		8)saveCPURegesters():Back up the CPU regesters into the current PCB
 *		9)loadCPURegesters():load backed upPCB regesters into the CPU.
 *		10)JobSuspendCPU():Context switch when 40 clock cycles are executed
 *		11)JobSuspendIO():Context switch hen IO occures.
 *		12)printMLFBQ(): Prints contents of queue into MLFBQ file, called 
 *		every 1200 CLOCK units
 *
 */

// Declaring System calls for IO operations.
void systemI(int, int);
void systemO(int, int);

// A is Accumlator. 16, 32-bit regesters.
// MBR is memory Buffer Register
unsigned long A[17];
unsigned long MBR[4];

int number1, number2;

bool inBlockedQueue(int jobID) {
  bool exist = false;
  if (jobID == -1)
    return false;
  else{
    while (!blockedQueue.empty()){
      if (blockedQueue.front().JobID == jobID)
        exist = true;
      tempQueue.push(blockedQueue.front());
      blockedQueue.pop();

    }
    while (!tempQueue.empty()){
      blockedQueue.push(tempQueue.front());
      tempQueue.pop();
    }
  }
  return exist;
}

void sampleSubqueueSize() {
  subqueuesizeSampleCount++;
  for (int i = 0; i < 4; i++){
    subqueuesize[i] += subQueue[i].size();
  }
}

void printMLFBQ() {
  ofstream myFS;
  string s = "MLFBQ";
  myFS.open(s.c_str(), ios_base::app);
  myFS << "\n\nCLOCK(hex) :" << hex << CLOCK;
  int lineControl = 5;
  for (int i = 0; i < 4; i++){
    myFS << "\nContents of sub-queue" << i + 1 << "(hex) :\n";
    if (subQueue[i].empty())
      myFS << "\t nil";
    else
      while (!subQueue[i].empty()){
        if (lineControl-- == 0){
          lineControl = 5;
          myFS << "\n";
        }
        myFS << "\t" << hex << subQueue[i].front().JobID;
        tempQueue.push(subQueue[i].front());
        subQueue[i].pop();

      }
    while (!tempQueue.empty()){
      subQueue[i].push(tempQueue.front());
      tempQueue.pop();
    }
  }
  myFS.close();
}

int GetRealNumber(unsigned long number) {

  unsigned long sign_bit = number / 0x10000000;
  int value = number % 0x10000000;

  if (sign_bit > 7){
    sign_bit = sign_bit - 0x8;
    value = (value + (sign_bit * 0x10000000));
    value = -value;
  }else{
    value = (int) number;
  }

  return value;
}

unsigned long GetSystemNumber(int number) {

  int sign_Bit = 0;
  unsigned long value;
  if (number < 0){
    sign_Bit = 8;
    number = number * -1;
  }

  value = sign_Bit * 0x10000000;
  value = value + number;

  return value % 0x100000000;
}

void beforeExeTrace(int PC, int Inst, unsigned long A, int Y) {
  if (Y == 1){
    ofstream myFS;
    string s = numberToString(currentJobPCB.JobID);
    myFS.open(s.c_str(), ios_base::app);
    myFS << " " << hex << PC << "\t\t" << hex << setw(8) << Inst << "\t" << hex
      << setw(8) << A % 0x100000000 << "\t";
  }
}
void afterExeTrace(int EA, unsigned long A, int Y) {
  if (Y == 1){
    ofstream myFS;
    string s = numberToString(currentJobPCB.JobID);
    myFS.open(s.c_str(), ios_base::app);
    myFS << hex << setw(8) << A % 0x100000000 << "\t" << hex << EA << "\n";
  }
}

void haltPrint() {
  ofstream myFS;

  myFS.open(FN[0], ios_base::app);
  myFS << "\n\n CLOCK(dec):\t\t\t" << CLOCK << "\n JOBID(hex):\t\t\t" << hex
    << currentJobPCB.JobID << "\n Clock at load time(hex):\t" << hex
    << currentJobPCB.ClockAtLoadTime << "\n Clock at termination(hex):\t" << hex
    << CLOCK << "\n";
  currentJobPCB.DiskPageReadAddress = 0;
  if (currentJobPCB.numberOfWriteAvailable2 != 0)
    myFS << " Output(hex):\n\t";
  while (currentJobPCB.numberOfWriteAvailable2-- != 0){
    myFS << setfill('0') << setw(8) << hex
      << DISK[calculatePhysicalLocationOnDiskE(currentJobPCB.DiskPagesForWrite,
        currentJobPCB.DiskPageReadAddress++)];
    if (currentJobPCB.DiskPageReadAddress % 4 == 0
      && currentJobPCB.numberOfWriteAvailable2 != 0)
      myFS << "\n\t";
  }
  myFS << "\n Nature of termination:\t\tNormal";
  myFS << "\n Turnaround time(hex):\t\t" << hex
    << (currentJobPCB.ClockAtTerminationTime - currentJobPCB.ClockAtLoadTime);
  myFS << "\n Run time(hex):\t\t\t" << hex << currentJobPCB.TimeTaken;
  myFS << "\n Execution time(hex):\t\t" << hex
    << currentJobPCB.TimeTaken - currentJobPCB.TimeOfCompletionOfIO;
  myFS << "\n Page fault handling time(hex):\t" << currentJobPCB.PageFaultTime;
  myFS << "\n Number of CPU turns(dec):\t" << currentJobPCB.CPUShots;
  totalTurnAroundTime += (currentJobPCB.ClockAtTerminationTime
    - currentJobPCB.ClockAtLoadTime);
  if (currentJobPCB.TimeTaken - currentJobPCB.TimeOfCompletionOfIO > 0)
    totalExecutionTime += (currentJobPCB.TimeTaken
      - currentJobPCB.TimeOfCompletionOfIO);
  else
    totalExecutionTime += -(currentJobPCB.TimeTaken
      - currentJobPCB.TimeOfCompletionOfIO);

  totalIOTime += currentJobPCB.TimeOfCompletionOfIO;
  totalRunTime += currentJobPCB.TimeTaken;
  myFS.close();

}

void loadCPURegisters(PCB currentJPCB) {
  for (int i = 0; i < 17; i++){
    A[i] = currentJPCB.Registers[i];
  }
}
void saveCPURegisters(PCB &currentJPCB) {
  for (int i = 0; i < 17; i++){
    currentJPCB.Registers[i] = A[i];
  }
}

void JobSuspendCPU(PCB suspendJobPCB, int timeTaken, int PC) {

  saveCPURegisters(suspendJobPCB);
  suspendJobPCB.TimeTaken = suspendJobPCB.TimeTaken + timeTaken;
  if (suspendJobPCB.QueueIndex == 3){
    suspendJobPCB.timeSpentIn4thSubQueue += timeTaken;
  }
  suspendJobPCB.PC = PC;
  suspendJobPCB.ClockAtEnterBlockedQueue = CLOCK;

  // If job exists in 4th queue OR it still has turns left in current queue
  if (suspendJobPCB.QueueIndex == 3
    || suspendJobPCB.n <= (n + (suspendJobPCB.QueueIndex * 2))){
    if (suspendJobPCB.QueueIndex == 3
      && suspendJobPCB.timeSpentIn4thSubQueue >= 9 * n * q){
      suspendJobPCB.n = 0;
      suspendJobPCB.QueueIndex = 0;
      suspendJobPCB.timeSpentIn4thSubQueue = 0;
      migrationsInSubqueueCount++;
      subQueue[0].push(suspendJobPCB);
    }else{
      subQueue[suspendJobPCB.QueueIndex].push(suspendJobPCB);
    }
  }else{
    suspendJobPCB.n = 0;
    suspendJobPCB.QueueIndex += 1;
    migrationsInSubqueueCount++;
    subQueue[suspendJobPCB.QueueIndex].push(suspendJobPCB);
  }

  return;
}

void JobSuspendIO(PCB suspendJobPCB, int timeTaken, int PC) {

  saveCPURegisters(suspendJobPCB);
  suspendJobPCB.TimeTaken = suspendJobPCB.TimeTaken + timeTaken;
  suspendJobPCB.PC = PC + 1;
  suspendJobPCB.ClockAtEnterBlockedQueue = CLOCK;
  blockedQueue.push(suspendJobPCB);
  return;

}

void regularIntervalPrint() {

  int lineControl = 5;
  ofstream myFS;
  myFS.open(FN[0], ios_base::app);
  myFS << "\n\n CLOCK(dec): " << CLOCK;

  myFS << "\n Status of operating system";
  int readyQueuSize = subQueue[0].size() + subQueue[1].size()
    + subQueue[2].size() + subQueue[3].size();
  if (!subQueue[0].empty() || !subQueue[1].empty() || !subQueue[2].empty()
    || !subQueue[3].empty())
    myFS << "\n Ready Queue Job ID's(hex):\n";
  lineControl = 5;
  for (int i = 0; i < readyQueuSize; i++){
    if (readyQueueJobIDs[i] == -1 || readyQueueJobIDs[i] == currentJobPCB.JobID
      || inBlockedQueue(readyQueueJobIDs[i])){
      readyQueuSize++;
    }else{
      myFS << "\t" << hex << readyQueueJobIDs[i];
      if (lineControl-- == 0 && i != 1){
        myFS << "\n";
        lineControl = 5;
      }
    }
  }
  readyQueuSize = subQueue[0].size() + subQueue[1].size() + subQueue[2].size()
    + subQueue[3].size();
  myFS << "\n Currently executing jobid(hex):\t\t" << hex
    << currentJobPCB.JobID;

  lineControl = 5;
  if (!blockedQueue.empty())
    myFS << "\n Blocked Queue Job ID's(hex):\n";
  while (!blockedQueue.empty()){
    tempQueue.push(blockedQueue.front());
    myFS << "\t" << hex << blockedQueue.front().JobID;
    if (lineControl-- == 0 && blockedQueue.size() != 1){
      myFS << "\n";
      lineControl = 5;
    }
    blockedQueue.pop();
  }
  while (!tempQueue.empty()){
    blockedQueue.push(tempQueue.front());
    tempQueue.pop();
  }
  myFS << "\n Current degree of multiprogramming(dec):\t"
    << (blockedQueue.size() + subQueue[0].size() + subQueue[1].size()
      + subQueue[2].size() + subQueue[3].size() + 1);

  myFS << "\n Percentage of Disk in use(dec):\t\t" << setprecision(4)
    << ((1 - ((double) numberOfEmptyPagesOnDisk() / 256)) * 100);

  DiskHoleCount += numberOfEmptyPagesOnDisk();
  DiskCount += (256 - numberOfEmptyPagesOnDisk());
  DiskHoleSampleCount++;

  myFS << "\n Current Memory Configuration";

  myFS << "\n Current jobs on Memory(hex):\n";
  lineControl = 5;
  for (int i = 0; i < readyQueuSize + 1 + blockedQueue.size(); i++){
    if (readyQueueJobIDs[i] == -1){
      readyQueuSize++;
    }else{
      myFS << "\t" << hex << readyQueueJobIDs[i];
      if (lineControl-- == 0 && i != 1){
        myFS << "\n";
        lineControl = 5;
      }
    }
  }

  myFS << "\n Holes on Memory(dec):\t\t\t\t" << numberOfEmptyPagesOnMemory();
  myFS.close();
}

void CPU(int X, int Y) {

// EA is memoryAddressRegister only one needed.
  int EA, PC;

  int jobEnterTime = CLOCK;
  currentJobPCB.n++;
  currentJobPCB.CPUShots++;
  ofstream myFS2;
  PC = X;
  unsigned long instruction;
  int IOP, AR, BI, DADDR;

  ofstream myFS;

  while (true){

    if (CLOCK - jobEnterTime >= (n + (currentJobPCB.QueueIndex * 2)) * q){
      afterExeTrace(EA, A[AR], Y);
      JobSuspendCPU(currentJobPCB, (CLOCK - jobEnterTime), PC);
      return;
    }
    // Fetch instruction
    MEMORY("READ", PC, MBR[0]);
    instruction = MBR[0];

    // holds I + OPCODE
    IOP = (int) (instruction / 0x01000000);
    // holds A
    AR = (int) (instruction / 0x00100000) - (IOP * 0x00000010);
    // holds B
    BI = (int) (instruction / 0x00010000) - (IOP * 0x00000100)
      - (AR * 0x00000010);
    // holds DADDR
    DADDR = (int) (instruction % 0x00010000);

    if (AR > 0x0f || BI > 0x0f){
      // Register address error
      ERROR(403);
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      return;
    }

    // Addressing modes :

    EA = DADDR;
    // with indirect addressing: EA = C(DADDR)
    if (IOP >= 0x80 && BI == 0x00){
      MEMORY("READ", EA, MBR[0]);
      EA = MBR[0];
      IOP = IOP - 0x80;
    }

    // with index addressing : EA = INDEX + DADDR // Check this
    if (BI > 0x00 && IOP < 0x80){

      EA = A[BI] + DADDR;

    }

    // with both: EA = INDEX + C(DADDR)
    if (BI > 0x00 && IOP >= 0x80){
      MEMORY("READ", EA, MBR[0]);
      EA = A[BI] + MBR[0];
      IOP = IOP - 0x80;
    }

    if (EA > 0xFF && EA > currentJobPCB.programLength){
      // Memory address fault
      ERROR(301);
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      return;
    }

    //Trace output before execution
    beforeExeTrace(PC, instruction, A[AR], Y);

    // Execute the appropriate instruction
    switch (IOP) {

    case 0x00: //HALT

      CLOCK++;
      numberOfNormalJobs++;
      currentJobPCB.TimeTaken += CLOCK - jobEnterTime;
      currentJobPCB.ClockAtTerminationTime = CLOCK;
      afterExeTrace(EA, A[AR], Y);
      haltPrint();
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      LOADER();
      DiskLoad();

      return;

    case 0x01: //LOAD
      MEMORY("READ", EA, MBR[0]);
      CLOCK++;
      A[AR] = MBR[0];
      break;

    case 0x02: //STORE
      MEMORY("WRIT", EA, A[AR]);
      CLOCK++;
      break;

    case 0x03: //ADD
      MEMORY("READ", EA, MBR[0]);
      CLOCK++;
      number1 = GetRealNumber(A[AR]);
      number2 = GetRealNumber(MBR[0]);
      if (number1 + number2 <= 0x7FFFFFFF && number1 + number2 >= -0x7FFFFFFF){
        A[AR] = GetSystemNumber(number1 + number2);
      }else{
        ERROR(404);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }
      break;

    case 0x04: //Subtract

      MEMORY("READ", EA, MBR[0]);
      CLOCK++;

      number1 = GetRealNumber(A[AR]);
      number2 = GetRealNumber(MBR[0]);

      if (number1 - number2 <= 0x7FFFFFFF && number1 - number2 >= -0x7FFFFFFF){
        A[AR] = GetSystemNumber(number1 - number2);
      }else{
        // CPU Register underflow
        afterExeTrace(EA, A[AR], Y);
        ERROR(407);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }
      break;

    case 0x05: //Multiply

      MEMORY("READ", EA, MBR[0]);
      number1 = GetRealNumber(A[AR]);
      number2 = GetRealNumber(MBR[0]);
      CLOCK++;
      CLOCK++;
      if (number1 * number2 <= 0x7FFFFFFF && number1 * number2 >= -0x7FFFFFFF){
        A[AR] = GetSystemNumber(number1 * number2);
      }else{
        ERROR(404);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        afterExeTrace(EA, A[AR], Y);
        return;
      }
      break;

    case 0x06: //Divide
      MEMORY("READ", EA, MBR[0]);
      number1 = GetRealNumber(A[AR]);
      number2 = GetRealNumber(MBR[0]);
      CLOCK++;
      CLOCK++;
      if (number2 == 0){
        afterExeTrace(EA, A[AR], Y);
        ERROR(405);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }
      if (number1 / number2 <= 0x7FFFFFFF && number1 / number2 >= -0x7FFFFFFF){
        A[AR] = GetSystemNumber(number1 / number2);
      }else{
        afterExeTrace(EA, A[AR], Y);
        ERROR(405);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }
      break;

    case 0x07: // Bitwise shift left
      MEMORY("READ", EA, MBR[0]);
      CLOCK++;
      A[AR] = A[AR] << MBR[0];
      A[AR] = A[AR] % 0x100000000;
      break;

    case 0x08: //Bitwise shift right
      MEMORY("READ", EA, MBR[0]);
      CLOCK++;
      A[AR] = A[AR] >> MBR[0];
      A[AR] = A[AR] % 0x100000000;
      break;

    case 0x09: //Branch on minus
      CLOCK++;
      if (GetRealNumber(A[AR]) < 0)
        PC = EA - 1;
      break;

    case 0x0A: // Branch on Plus
      if (GetRealNumber(A[AR]) > 0)
        PC = EA - 1;
      CLOCK++;
      break;

    case 0x0B: // Branch on zero
      if (GetRealNumber(A[AR]) == 0)
        PC = EA - 1;
      CLOCK++;
      break;

    case 0x0C: // Branch and link
      MEMORY("READ", PC, MBR[0]);
      CLOCK++;
      CLOCK++;
      A[AR] = MBR[0];
      PC = EA - 1;
      break;

    case 0x0D: // Bitwise AND
      MEMORY("READ", EA, MBR[0]);
      A[AR] = A[AR] ^ MBR[0];
      A[AR] = A[AR] % 0x100000000;
      CLOCK++;
      break;

    case 0x0E: // Bitwise OR
      MEMORY("READ", EA, MBR[0]);
      A[AR] = A[AR] | MBR[0];
      A[AR] = A[AR] % 0x100000000;
      CLOCK++;
      break;

    case 0x0F: //READ

      CLOCK++;
      CLOCK++;
      afterExeTrace(EA, A[AR], Y);

      systemI(jobEnterTime, PC);

      if (exit_code == 1){
        ERROR(409);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }
      // return
      MEMORY("WRIT", EA, MBR[0]);
      MEMORY("WRIT", EA + 1, MBR[1]);
      MEMORY("WRIT", EA + 2, MBR[2]);
      MEMORY("WRIT", EA + 3, MBR[3]);

      //if(readyQueue.size() != 0)
      return;

      break;
    case 0x10: // Write

      CLOCK++;
      CLOCK++;
      MEMORY("READ", EA, MBR[0]);
      MEMORY("READ", EA + 1, MBR[1]);
      MEMORY("READ", EA + 2, MBR[2]);
      MEMORY("READ", EA + 3, MBR[3]);
      afterExeTrace(EA, A[AR], Y);

      systemO(jobEnterTime, PC);

      if (exit_code == 1){
        ERROR(408);
        SpoolFromMemory(currentJobPCB);
        SpoolFromDisk(currentJobPCB.JobID);
        return;
      }

      //if(readyQueue.size() != 0)
      return;

      break;

    case 0x11: //Dump
      MEMORY("DUMP", 0, MBR[0]);
      CLOCK++;
      break;

    default:
      // OPCODE ERROR
      ERROR(401);
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      return;
      break;
    }

    // Trace output after execution
    afterExeTrace(EA, A[AR], Y);
    PC++;
    if (PC > 0xFF){
      ERROR(402);
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      return;
    }
    if ((CLOCK - jobEnterTime) + currentJobPCB.TimeTaken > 0x100000){ // More than 5000 clock cycles.
      // Suspect infinite job.
      ERROR(406);
      totalSuspectInfiniteJobtime += (CLOCK - jobEnterTime)
        + currentJobPCB.TimeTaken;
      InfiniteJobIDs.push(currentJobPCB.JobID);
      SpoolFromMemory(currentJobPCB);
      SpoolFromDisk(currentJobPCB.JobID);
      LOADER();
      DiskLoad();
      return;
    }

    while (!blockedQueue.empty()
      && (CLOCK - blockedQueue.front().IOTimeStamp) >= 8){
      if (blockedQueue.front().QueueIndex == 3){
        blockedQueue.front().n = 0;
        blockedQueue.front().QueueIndex = 0;
        blockedQueue.front().timeSpentIn4thSubQueue = 0;
        migrationsInSubqueueCount++;
        subQueue[0].push(blockedQueue.front());
        blockedQueue.pop();
      }else{
        blockedQueue.front().n = 0;
        subQueue[blockedQueue.front().QueueIndex].push(blockedQueue.front());
        blockedQueue.pop();
      }
    }

    if (CLOCK > flag_reguralInterval * 0x2500){
      flag_reguralInterval++;
      regularIntervalPrint();
    }
    if (CLOCK > fiveHCount * 500){
      fiveHCount++;
      sampleSubqueueSize();
    }
    if (CLOCK > countMLFBQinterval * 1200){
      countMLFBQinterval++;
      // Print contents of subqueues into file
      printMLFBQ();
    }

  }

}
