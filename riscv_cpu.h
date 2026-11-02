#ifndef RISCV_CPU_H
#define RISCV_CPU_H

#include <ap_int.h>

// Memory sizes (in 32-bit words). Must be powers of 2.
#define IMEM_WORDS 256      // 1 KB instruction memory
#define DMEM_WORDS 256      // 1 KB data memory

// Phase 1: execution unit (one instruction, no state)
void riscv_cpu(
    ap_uint<32> instruction,
    ap_uint<32> rs1_value,
    ap_uint<32> rs2_value,
    ap_uint<32> pc,
    ap_uint<32> &rd_value,
    ap_uint<5>  &rd_index,
    ap_uint<1>  &write_enable,
    ap_uint<32> &mem_address,
    ap_uint<32> &mem_write_data,
    ap_uint<1>  &mem_write_enable,
    ap_uint<2>  &mem_write_size,
    ap_uint<1>  &branch_taken,
    ap_uint<32> &branch_target,
    ap_uint<32> &mem_read_address,
    ap_uint<1>  &mem_read_enable,
    ap_uint<2>  &mem_read_size,
    ap_uint<32> mem_read_data
);

// Phase 2: full CPU (runs a whole program)
void riscv_top(
    const ap_uint<32> imem[IMEM_WORDS],
    ap_uint<32>       dmem[DMEM_WORDS],
    ap_uint<32>       max_steps,
    ap_uint<32>       regs_out[32],
    ap_uint<32>       &final_pc,
    ap_uint<32>       &steps_done,
    ap_uint<1>        &halted
);

// Phase 3: FPGA top level (VIO control: reset / step / run + debug outputs)
void riscv_fpga(
    const ap_uint<32> imem[IMEM_WORDS],
    ap_uint<32>       dmem[DMEM_WORDS],
    ap_uint<1>        btn_reset,
    ap_uint<1>        btn_step,
    ap_uint<1>        btn_run,
    ap_uint<5>        reg_sel,
    ap_uint<32>       &dbg_pc,
    ap_uint<32>       &dbg_instr,
    ap_uint<32>       &dbg_reg,
    ap_uint<32>       &dbg_count,
    ap_uint<1>        &dbg_halted
);

// FINAL FPGA TOP: memories inside, program from program.h
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
);

#endif
