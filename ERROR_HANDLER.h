/*
 * ERROR_HANDLER.h
 *
 *
 *	Description:
 *		Errorhandler can be called cfrom any module other modules. To print the necessary
 *	error to user.
 *	Global variables:
 *		error_code : sent by the module which specifies error type and description.
 * 		exit_code: specifies type of exit.
 *	Functions:
 *		checkExitCode(): Used by other modules to check if system has run into error.
 *		ERROR() : Takes error code and prints the specifiesd error.
 */
#include<iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <iomanip>

int error_code, exit_status, exit_code = 0;

string numberToString(int i) {
  std::stringstream s;
  s << hex << i;
  return s.str();
}

int calculatePhysicalLocationOnDiskE(unsigned short pageTable[20],
  int location) {

  int pageTableIndex = location / 0x10;
  location = location % 0x10;

  location = location + (pageTable[pageTableIndex] * 0x10);

  return location;
}

bool checkExitCode() {

  ofstream myFS;
  myFS.open(FN[0], ios_base::app);

  // Print error exit type or return no error.
  if (exit_code == 1){
    myFS << "\n\nSystem exit with exit(1).\n";
    return true;
  }else if (exit_code == -1){
    myFS << "\n\nSystem exit with few warnings, exit(-1).\n";
    return true;
  }else{
    return false;
  }

  myFS.close();
  return false;
}

void ERROR(int err_code) {

  currentJobPCB.ClockAtTerminationTime = CLOCK;

  //100s - load time errors
  //200s - decoding time errors
  //300s - memory reference errors
  //400s - execution time errors
  //500s - system errors

  //150s - load time warnings //loader
  //250s - decoding time warnings //cpu
  //350s - memory reference warnings//memory
  //450s - execution time warnings//cpu
  //550S - SYSTEM WARNIGS

  string errors[600];

  errors[1] = "LOAD";
  errors[2] = "DECODE";
  errors[3] = "MEMORY";
  errors[4] = "EXECUTION";
  errors[5] = "SYSTEM";

  errors[0] = "ERROR :";
  errors[50] = "WARNING :";

  // Errors

  errors[501] = "Input mismatch error";
  errors[502] = "Output mismatch error";

  errors[100] = "Invalid file.";
  errors[101] = "Length of program too long.";
  errors[102] = "Program length mismatch.";
  errors[103] = "Bad Character Encountered.";
  //phase 2
  errors[104] = "Double **JOB";
  errors[105] = "Missing **FIN";
  errors[106] = "**JOB ## ## format invalid";
  errors[107] = "Double **DATA";
  errors[108] = "Missing **JOB";
  errors[109] = "Null JOB";
  errors[110] = "Double **FIN";
  errors[111] = "Missing **DATA";
  errors[112] = "Buffer Overflow";
  errors[113] = "MISSING PROGRAM";
  errors[114] = "PROGRAM TOO LONG";
  errors[115] = "Insifficent Pages for DATA";
  errors[116] = "DATA OVERFLOW";
  errors[117] = "MISSING **JOB";

  errors[301] = "Address out of range.";

  errors[401] = "Invalid opcode.";
  errors[402] = "Program counter out of range.";
  errors[403] = "Register Address out of range.";
  errors[404] = "Register overflow.";
  errors[405] = "Attempt to divide by 0";
  errors[406] = "Suspect infinite job.";
  errors[407] = "Register underflow.";
  //phase2
  errors[408] = "INSUFFICIENT OUTPUT PAGES";
  errors[409] = "INSUFFICIENT DATA PAGES";

  //Warnings

  errors[550] =
    "Start Address out of bounds. Setting PC to first instructions location";
  errors[551] = "Trace Bit Invalid, Setting to 0";

  errors[150] = "Location out of bounds. Setting to 00.";
  errors[151] = "Input file line length too large. Ignoring extra characters.";
  errors[152] = "Pages unused.";

  string type = errors[err_code / 100];
  // Get error type and description.
  string err_war;
  if ((err_code - (100 * (err_code / 100))) >= 50){
    err_war = errors[50];
    exit_code = -1;
  }else{
    err_war = errors[0];
    exit_code = 1;
    if (numberOfAbnormalJobs == exit_code && flag == 0){
      numberOfAbnormalJobs -= exit_code * 4;
      flag = 1;
    }
  }
  ofstream myFS;
  if (err_code / 100 == 1){
    if (err_war == errors[0])
      numberOfAbnormalJobs++;
    myFS.open(FN[0], ios_base::app);
    myFS << "\n\n CLOCK(dec): " << CLOCK << "\tJOBID(hex) : " << hex
      << currentJobPCB.JobID << "\t" << type << " : " << err_war << " ["
      << err_code << "] : " << errors[err_code];
    myFS.close();
  }else if (err_code / 100 == 4){

    myFS.open(FN[0], ios_base::app);
    myFS << "\n\n CLOCK(dec):\t\t\t\t\t" << CLOCK
      << "\n JOBID(hex) : \t\t\t\t\t" << hex << currentJobPCB.JobID;
    myFS << "\n Clock at load time(hex) : \t\t\t" << hex
      << currentJobPCB.ClockAtLoadTime;
    myFS << "\n Clock at termination time(hex) : \t\t" << hex << CLOCK << "\n";
    myFS << " " << type << " : " << err_war << " [" << err_code << "] : "
      << errors[err_code];
    currentJobPCB.DiskPageReadAddress = 0;
    if (currentJobPCB.numberOfWriteAvailable2 != 0)
      myFS << " Output(hex:)\n\t";
    while (currentJobPCB.numberOfWriteAvailable2-- != 0){
      myFS << setfill('0') << setw(8) << hex
        << DISK[calculatePhysicalLocationOnDiskE(currentJobPCB.DiskPagesForRead,
          currentJobPCB.DiskPageReadAddress++)];
      if (currentJobPCB.DiskPageReadAddress % 4 == 0)
        myFS << "\t\n";
    }
    myFS << "\n Nature of termination : \t\t\tAbnormal";
    myFS << "\n Turnaround time(hex) : \t\t\t" << hex
      << (currentJobPCB.ClockAtTerminationTime - currentJobPCB.ClockAtLoadTime);
    myFS << "\n Run time(hex) : \t\t\t\t" << hex << currentJobPCB.TimeTaken;
    myFS << "\n Execution time(partial)(hex) : \t\t" << hex
      << currentJobPCB.TimeTaken - currentJobPCB.TimeOfCompletionOfIO;
    myFS << "\n Page fault time(hex) : \t\t\t" << currentJobPCB.PageFaultTime
      << "\n";
    if (err_war != "WARNING :")
      numberOfAbnormalJobs++;
    totalErrorJobTime += currentJobPCB.TimeTaken;
    totalTurnAroundTime += (currentJobPCB.ClockAtTerminationTime
      - currentJobPCB.ClockAtLoadTime);
    totalExecutionTime += (currentJobPCB.TimeTaken
      - currentJobPCB.TimeOfCompletionOfIO);
    totalIOTime += currentJobPCB.TimeOfCompletionOfIO;
    totalRunTime += currentJobPCB.TimeTaken;

    myFS.close();
  }

  return;
}
