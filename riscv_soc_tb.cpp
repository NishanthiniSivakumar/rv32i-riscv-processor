// =====================================================================
// riscv_soc_tb.cpp - Testbench for the final FPGA top (riscv_soc)
// Uses the DEFAULT program.h (demo_bubblesort.asm). If you change
// program.h, the expected values below no longer apply.
//
// Acts like VIO: RESET, STEP through the whole program checking the PC
// after every instruction, read all registers and the sorted array,
// then RESET + RUN and check the same results.
// =====================================================================
#include <iostream>
#include <iomanip>
#include "riscv_cpu.h"
using namespace std;

static const unsigned TRACE[] = { 0x00000004, 0x00000008, 0x0000000C, 0x00000010, 0x00000014, 0x00000018, 0x0000001C, 0x00000020, 0x00000024, 0x00000028, 0x0000002C, 0x00000030, 0x00000034, 0x00000038, 0x0000003C, 0x00000040, 0x00000044, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000006C, 0x00000070, 0x0000003C, 0x00000040, 0x00000044, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000006C, 0x00000070, 0x0000003C, 0x00000040, 0x00000044, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000006C, 0x00000070, 0x0000003C, 0x00000040, 0x00000044, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000006C, 0x00000070, 0x0000003C, 0x00000040, 0x00000044, 0x00000048, 0x0000004C, 0x00000050, 0x00000054, 0x00000058, 0x0000005C, 0x00000060, 0x00000064, 0x00000068, 0x00000048, 0x0000006C, 0x00000070, 0x0000003C, 0x00000074, 0x00000078, 0x00000078 };
static const unsigned REGS[] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFF8, 0xFFFFFFFD, 0xFFFFFFF8, 0x00000000, 0x00000000, 0x00002000, 0x00000006, 0x00000000, 0x00000001, 0x00002004, 0x00000000, 0x00000000, 0x0000000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static const unsigned MEM[] = { 0xFFFFFFF8, 0xFFFFFFFD, 0x00000000, 0x00000005, 0x00000007, 0x0000000C, 0x00000000, 0x00000000 };
static const int STEPS = 171;

static ap_uint<32> pc_o, instr_o, reg_o, count_o, mem_o;
static ap_uint<1>  halted_o;
static int errors = 0, checks = 0;

void call(int rst, int step, int run, int rsel = 0, int msel = 0) {
    riscv_soc(rst, step, run, rsel, msel, pc_o, instr_o, reg_o, count_o, halted_o, mem_o);
}
void press_reset() { call(1, 0, 0); call(0, 0, 0); }
void press_step()  { call(0, 1, 0); call(0, 0, 0); }
void press_run()   { call(0, 0, 1); call(0, 0, 0); }

void check(const char* what, unsigned got, unsigned exp, int idx = -1) {
    checks++;
    if (got != exp) {
        errors++;
        cout << "  FAIL " << what; if (idx >= 0) cout << "[" << idx << "]";
        cout << hex << ": got 0x" << got << " expected 0x" << exp << dec << endl;
    }
}

void check_final(const char* phase) {
    int e0 = errors;
    check("halted", (unsigned)halted_o, 1);
    check("count", (unsigned)count_o, STEPS);
    check("pc", (unsigned)pc_o, TRACE[STEPS - 1]);
    for (int r = 0; r < 32; r++) { call(0, 0, 0, r, 0); check("x", (unsigned)reg_o, REGS[r], r); }
    for (int m = 0; m < 8; m++)  { call(0, 0, 0, 0, m); check("mem", (unsigned)mem_o, MEM[m], m); }
    cout << "  " << phase << ": " << ((errors == e0) ? "PASS" : "FAIL") << endl;
}

int main() {
    cout << "riscv_soc testbench (demo: bubble sort, ends with ECALL)" << endl;

    press_reset();
    check("reset pc", (unsigned)pc_o, 0);
    check("reset count", (unsigned)count_o, 0);
    for (int s = 0; s < STEPS; s++) {
        press_step();
        check("step pc", (unsigned)pc_o, TRACE[s], s);
        check("step count", (unsigned)count_o, s + 1, s);
    }
    press_step();                                   // after halt: nothing changes
    check_final("STEP mode, every instruction");

    press_reset();
    check("reset again: halted", (unsigned)halted_o, 0);
    press_run();
    check_final("RUN mode, whole program");

    cout << endl << "sorted array (dbg_mem 0..5): ";
    for (int m = 0; m < 6; m++) { call(0, 0, 0, 0, m); cout << (int)(ap_int<32>)mem_o << " "; }
    cout << endl << "Checks: " << checks << "   Failed: " << errors << endl;
    return errors == 0 ? 0 : 1;
}
