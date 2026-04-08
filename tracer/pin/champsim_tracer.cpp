// champsim_tracer.cpp
/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs
 *  and could serve as the starting point for developing your first PIN tool
 */

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "../../inc/trace_instruction.h"
#include "pin.H"

using trace_instr_format_t = input_instr;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 instrCount = 0;

std::ofstream outfile;

trace_instr_format_t curr_instr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "champsim.trace", "specify file name for Champsim tracer output");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "1000000", "How many instructions to trace");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
  std::cerr << "This tool creates a register and memory access trace" << std::endl
            << "Specify the output trace file with -o" << std::endl
            << "Specify the number of instructions to skip before tracing with -s" << std::endl
            << "Specify the number of instructions to trace with -t" << std::endl
            << std::endl;

  std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;

  return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void ResetCurrentInstruction(VOID* ip)
{
  curr_instr = {};
  curr_instr.ip = (unsigned long long int)ip;
}

BOOL ShouldWrite()
{
  ++instrCount;
  return (instrCount > KnobSkipInstructions.Value()) && (instrCount <= (KnobTraceInstructions.Value() + KnobSkipInstructions.Value()));
}

void WriteCurrentInstruction()
{
  typename decltype(outfile)::char_type buf[sizeof(trace_instr_format_t)];
  std::memcpy(buf, &curr_instr, sizeof(trace_instr_format_t));
  outfile.write(buf, sizeof(trace_instr_format_t));
}

void BranchOrNot(UINT32 taken)
{
  curr_instr.is_branch = 1;
  curr_instr.branch_taken = taken;
}

template <typename T>
void WriteToSet(T* begin, T* end, UINT64 r)
{
  auto set_end = std::find(begin, end, 0);
  auto found_reg = std::find(begin, set_end, r); // check to see if this register is already in the list
  *found_reg = r;
}

#include <iostream>
void WriteEvent(unsigned char type, uint64_t addr, uint64_t arg0, uint64_t arg1, uint32_t tid) {
    trace_instr_format_t ev;
    memset(&ev, 0, sizeof(ev));

    ev.is_malloc = type;

    ev.destination_memory[0] = addr;
    ev.source_memory[0] = arg0;
    ev.source_memory[1] = arg1;

    // 如果需要线程ID，可放入 source_memory[1] 或某个寄存器数组
    // ev.source_memory[1] = tid;

    outfile.write(reinterpret_cast<const char*>(&ev), sizeof(ev));
}

uint64_t tls_malloc_size = 0;

uint64_t tls_calloc_nmemb = 0;
uint64_t tls_calloc_size  = 0;

uint64_t tls_realloc_old_ptr = 0;
uint64_t tls_realloc_size    = 0;

void MallocEntry(THREADID tid, uint64_t size) {
    tls_malloc_size = size;
}
void MallocExit(THREADID tid, uint64_t ret_val) {
    if (ret_val != 0) {
        WriteEvent(INSTR_MALLOC, ret_val, tls_malloc_size, 0, tid);
    }
}

/* ================= calloc ================= */
void CallocEntry(THREADID tid, uint64_t nmemb, uint64_t size) {
    tls_calloc_nmemb = nmemb;
    tls_calloc_size  = size;
}
void CallocExit(THREADID tid, uint64_t ret_val) {
    if (ret_val != 0)
    {
        uint64_t total_size = tls_calloc_nmemb * tls_calloc_size;

        WriteEvent(
            INSTR_CALLOC,
            ret_val,
            total_size,
            0,
            tid
        );
    }
}

/* ================= realloc ================= */
void ReallocEntry(
    THREADID tid,
    uint64_t old_ptr,
    uint64_t size) 
{
    tls_realloc_old_ptr = old_ptr;
    tls_realloc_size    = size;
}
void ReallocExit(
    THREADID tid,
    uint64_t new_ptr)
{
    if (new_ptr != 0)
    {
        WriteEvent(
            INSTR_REALLOC,
            new_ptr,
            tls_realloc_size,
            tls_realloc_old_ptr,
            tid
        );
    }
}

void FreeEntry(THREADID tid, uint64_t ptr) {
    if (ptr != 0) {
        WriteEvent(INSTR_FREE, ptr, 0, 0, tid);  // free 大小设为0
    }
}

VOID ImageLoad(IMG img, VOID* v)
{
    // ===== hook malloc =====
    RTN mallocRtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(mallocRtn)) {
        RTN_Open(mallocRtn);

        RTN_InsertCall(
            mallocRtn,
            IPOINT_BEFORE,
            (AFUNPTR)MallocEntry,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);

        RTN_InsertCall(
            mallocRtn,
            IPOINT_AFTER,
            (AFUNPTR)MallocExit,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);

        RTN_Close(mallocRtn);
    }

    // ===== hook free =====
    RTN freeRtn = RTN_FindByName(img, "free");
    if (RTN_Valid(freeRtn)) {
        RTN_Open(freeRtn);

        RTN_InsertCall(
            freeRtn,
            IPOINT_BEFORE,
            (AFUNPTR)FreeEntry,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);

        RTN_Close(freeRtn);
    }

    /* ===================================================== */
    /* =================== calloc =========================== */
    /* ===================================================== */

    RTN callocRtn = RTN_FindByName(img, "calloc");
    if (RTN_Valid(callocRtn)) {

        RTN_Open(callocRtn);

        RTN_InsertCall(
            callocRtn,
            IPOINT_BEFORE,
            (AFUNPTR)CallocEntry,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, // nmemb
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1, // size
            IARG_END);

        RTN_InsertCall(
            callocRtn,
            IPOINT_AFTER,
            (AFUNPTR)CallocExit,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);

        RTN_Close(callocRtn);
    }

    /* ===================================================== */
    /* =================== realloc ========================== */
    /* ===================================================== */

    RTN reallocRtn = RTN_FindByName(img, "realloc");
    if (RTN_Valid(reallocRtn)) {

        RTN_Open(reallocRtn);

        RTN_InsertCall(
            reallocRtn,
            IPOINT_BEFORE,
            (AFUNPTR)ReallocEntry,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, // old ptr
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1, // size
            IARG_END);

        RTN_InsertCall(
            reallocRtn,
            IPOINT_AFTER,
            (AFUNPTR)ReallocExit,
            IARG_THREAD_ID,
            IARG_FUNCRET_EXITPOINT_VALUE,
            IARG_END);

        RTN_Close(reallocRtn);
    }
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

REG scratch_reg0 = PIN_ClaimToolRegister();
REG scratch_reg1 = PIN_ClaimToolRegister();

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID* v)
{
  // begin each instruction with this function
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ResetCurrentInstruction, IARG_INST_PTR, IARG_END);

  // instrument branch instructions
  if (INS_IsBranch(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BranchOrNot, IARG_BRANCH_TAKEN, IARG_END);

  // instrument register reads
  UINT32 readRegCount = INS_MaxNumRRegs(ins);
  for (UINT32 i = 0; i < readRegCount; i++) {
    UINT32 regNum = INS_RegR(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.source_registers, IARG_PTR,
                   curr_instr.source_registers + NUM_INSTR_SOURCES, IARG_UINT32, regNum, IARG_END);
  }

  // instrument register writes
  UINT32 writeRegCount = INS_MaxNumWRegs(ins);
  for (UINT32 i = 0; i < writeRegCount; i++) {
    UINT32 regNum = INS_RegW(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.destination_registers, IARG_PTR,
                   curr_instr.destination_registers + NUM_INSTR_DESTINATIONS, IARG_UINT32, regNum, IARG_END);
  }

  // instrument memory reads and writes
  UINT32 memOperands = INS_MemoryOperandCount(ins);

  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
    if (INS_MemoryOperandIsRead(ins, memOp))
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.source_memory, IARG_PTR,
                     curr_instr.source_memory + NUM_INSTR_SOURCES, IARG_MEMORYOP_EA, memOp, IARG_END);
      // printf("Instruction: %lld\n", curr_instr.source_memory[0]);
    if (INS_MemoryOperandIsWritten(ins, memOp))
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.destination_memory, IARG_PTR,
                     curr_instr.destination_memory + NUM_INSTR_DESTINATIONS, IARG_MEMORYOP_EA, memOp, IARG_END);
  }

  // if (INS_IsCall(ins))
  //   {
  //       // 获取函数名
  //       RTN rtn = INS_Rtn(ins);
  //       if (RTN_Valid(rtn))
  //       {
  //           std::string name = INS_Name(rtn);
  //           // malloc 插桩
  //           if (name == "malloc")
  //           {
  //               INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MallocEntry,
  //                              IARG_THREAD_ID,
  //                              IARG_FUNCARG_ENTRYPOINT_VALUE, 0, // size 参数
  //                              IARG_END);
  //               INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MallocExit,
  //                              IARG_THREAD_ID,
  //                              IARG_CONTEXT,
  //                              IARG_END);
  //           }
  //           // free 插桩
  //           if (name == "free")
  //           {
  //               INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)FreeEntry,
  //                              IARG_THREAD_ID,
  //                              IARG_FUNCARG_CALLSITE_VALUE, 0, // ptr
  //                              IARG_END);
  //           }
  //       }
  //   }


  // finalize each instruction with this function
  INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)ShouldWrite, IARG_END);
  INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteCurrentInstruction, IARG_END);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v) { outfile.close(); }

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
  PIN_InitSymbols();
  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid
  if (PIN_Init(argc, argv))
    return Usage();

  outfile.open(KnobOutputFile.Value().c_str(), std::ios_base::binary | std::ios_base::trunc);
  if (!outfile) {
    std::cout << "Couldn't open output trace file. Exiting." << std::endl;
    exit(1);
  }

  // Register function to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, 0);

  IMG_AddInstrumentFunction(ImageLoad, 0);

  // Register function to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
