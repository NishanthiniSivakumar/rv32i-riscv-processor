// =====================================================================
// riscv_soc.cpp  -  FINAL TOP for the FPGA (export this as the IP)
//
// Everything the processor needs is inside this one IP:
//   - instruction memory : ROM, filled from program.h (RARS machine code)
//   - data memory        : RAM, 256 words, starts at 0
//   - the CPU            : riscv_fpga() -> riscv_cpu()   (RV32I, 40 instr.)
//
// So the Vivado block design only needs: clock, this IP, VIO, constant 1.
//
//   VIO -> IP (inputs)                  IP -> VIO (outputs)
//   btn_reset  0->1 : reset CPU         dbg_pc     : current PC
//   btn_step   0->1 : run 1 instruction dbg_instr  : next instruction
//   btn_run    0->1 : run until halt    dbg_reg    : register x[reg_sel]
//   reg_sel    0..31: pick a register   dbg_count  : instructions executed
//   mem_sel    0..255: pick a data word dbg_halted : 1 = program finished
//                     (0 = addr 0x2000) dbg_mem    : data word dmem[mem_sel]
//
// To run another program: dump it from RARS, run make_program_h.py,
// replace program.h, then re-run C synthesis and Export RTL.
// =====================================================================
#include "riscv_cpu.h"
#include "program.h"

void riscv_soc(
    ap_uint<1>  btn_reset,
    ap_uint<1>  btn_step,
    ap_uint<1>  btn_run,
    ap_uint<5>  reg_sel,
    ap_uint<8>  mem_sel,
    ap_uint<32> &dbg_pc,
    ap_uint<32> &dbg_instr,
    ap_uint<32> &dbg_reg,
    ap_uint<32> &dbg_count,
    ap_uint<1>  &dbg_halted,
    ap_uint<32> &dbg_mem
) {
    // Debug outputs are plain wires (no handshake) so VIO can read them anytime
#pragma HLS INTERFACE mode=ap_none port=dbg_pc
#pragma HLS INTERFACE mode=ap_none port=dbg_instr
#pragma HLS INTERFACE mode=ap_none port=dbg_reg
#pragma HLS INTERFACE mode=ap_none port=dbg_count
#pragma HLS INTERFACE mode=ap_none port=dbg_halted
#pragma HLS INTERFACE mode=ap_none port=dbg_mem

    // Data memory lives inside the IP (becomes a block RAM).
    // "static" = keeps its contents between calls.
    static ap_uint<32> dmem[DMEM_WORDS];

    // One call: handle buttons, run 0 / 1 / many instructions, update debug
    riscv_fpga(PROGRAM_ROM, dmem,
               btn_reset, btn_step, btn_run, reg_sel,
               dbg_pc, dbg_instr, dbg_reg, dbg_count, dbg_halted);

    // Let VIO look at any data word (word index 0 = address 0x2000 in RARS)
    dbg_mem = dmem[mem_sel];
}
