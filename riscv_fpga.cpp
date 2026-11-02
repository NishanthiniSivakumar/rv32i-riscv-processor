// =====================================================================
// riscv_fpga.cpp  -  FPGA top level: final CPU (v1) + VIO control/debug
//
// Called by riscv_soc (the IP exported to Vivado); also tested on its own.
// It keeps the CPU state (PC, registers) between calls, so the CPU can be
// stepped one instruction at a time or run to the end, from VIO.
//
// In Vivado, ap_start of riscv_soc is tied to 1, so this runs again and again.
// Each call:
//   1. looks for a 0 -> 1 change (rising edge) on the VIO buttons
//   2. does the command:  RESET | STEP (1 instruction) | RUN (until halt)
//   3. updates the debug outputs that VIO displays
//
//   VIO buttons (inputs)          VIO probes (outputs)
//   -------------------          --------------------
//   btn_reset : PC = 0, regs = 0  dbg_pc     : current PC
//   btn_step  : run 1 instruction dbg_instr  : instruction at PC (next one)
//   btn_run   : run until halt    dbg_reg    : value of register reg_sel
//   reg_sel   : 0..31             dbg_count  : instructions executed
//                                 dbg_halted : 1 = program finished
// =====================================================================
#include "riscv_cpu.h"

#define RUN_LIMIT 1000000          // safety limit for RUN (program never halts)

void riscv_fpga(
    const ap_uint<32> imem[IMEM_WORDS],   // instruction memory (BRAM, loaded from .coe)
    ap_uint<32>       dmem[DMEM_WORDS],   // data memory (BRAM)
    ap_uint<1>        btn_reset,
    ap_uint<1>        btn_step,
    ap_uint<1>        btn_run,
    ap_uint<5>        reg_sel,
    ap_uint<32>       &dbg_pc,
    ap_uint<32>       &dbg_instr,
    ap_uint<32>       &dbg_reg,
    ap_uint<32>       &dbg_count,
    ap_uint<1>        &dbg_halted
) {
    // ---------------- CPU state: kept between calls ----------------
    // "static" = the value is remembered -> becomes flip-flops in hardware.
    static ap_uint<32> pc     = 0;
    static ap_uint<32> regs[32] = {0};
#pragma HLS ARRAY_PARTITION variable=regs complete
    static ap_uint<32> count  = 0;
    static ap_uint<1>  halted = 0;

    // Previous button values, for rising-edge detection
    static ap_uint<1> prev_reset = 0, prev_step = 0, prev_run = 0;

    // ---------------- 1. Detect button presses ----------------
    // A press = button was 0 last time and is 1 now. Holding a button at 1
    // does NOT repeat the command; set it back to 0 and to 1 again.
    bool do_reset = (btn_reset == 1) && (prev_reset == 0);
    bool do_step  = (btn_step  == 1) && (prev_step  == 0);
    bool do_run   = (btn_run   == 1) && (prev_run   == 0);
    prev_reset = btn_reset;
    prev_step  = btn_step;
    prev_run   = btn_run;

    // ---------------- 2. Do the command ----------------
    if (do_reset) {
        pc = 0;
        RESET_REGS: for (int i = 0; i < 32; i++) {
#pragma HLS UNROLL
            regs[i] = 0;
        }
        count  = 0;
        halted = 0;
    }

    // How many instructions to run now: 0 (nothing), 1 (step) or many (run).
    // One loop serves both modes, so the CPU datapath exists only once.
    ap_uint<32> limit = do_reset ? (ap_uint<32>)0 :
                        do_step  ? (ap_uint<32>)1 :
                        do_run   ? (ap_uint<32>)RUN_LIMIT : (ap_uint<32>)0;
    ap_uint<32> n = 0;

    EXEC: while (n < limit && halted == 0) {
#pragma HLS LOOP_TRIPCOUNT min=0 max=1000
#pragma HLS PIPELINE off
        // ===== same instruction cycle as the verified riscv_top (v1) =====

        // ---------- 1. FETCH ----------
        ap_uint<32> instr = imem[(pc >> 2) & (IMEM_WORDS - 1)];

        // ---------- 2. DECODE: read source registers ----------
        ap_uint<5>  rs1 = instr.range(19, 15);
        ap_uint<5>  rs2 = instr.range(24, 20);
        ap_uint<32> rs1_val = regs[rs1];
        ap_uint<32> rs2_val = regs[rs2];

        // ---------- 3. MEMORY READ (early, for loads) ----------
        ap_int<12>  ld_imm  = (ap_int<12>)instr.range(31, 20);
        ap_uint<32> ld_addr = rs1_val + (ap_int<32>)ld_imm;
        ap_uint<32> rd_word = dmem[(ld_addr >> 2) & (DMEM_WORDS - 1)];

        // ---------- 4. EXECUTE ----------
        ap_uint<32> rd_value, mem_address, mem_write_data, branch_target, mem_read_address;
        ap_uint<5>  rd_index;
        ap_uint<1>  write_enable, mem_write_enable, branch_taken, mem_read_enable;
        ap_uint<2>  mem_write_size, mem_read_size;

        riscv_cpu(instr, rs1_val, rs2_val, pc,
                  rd_value, rd_index, write_enable,
                  mem_address, mem_write_data, mem_write_enable, mem_write_size,
                  branch_taken, branch_target,
                  mem_read_address, mem_read_enable, mem_read_size,
                  rd_word);

        // ---------- 5. MEMORY WRITE (stores, read-modify-write) ----------
        if (mem_write_enable) {
            ap_uint<32> widx  = (mem_address >> 2) & (DMEM_WORDS - 1);
            ap_uint<2>  boff  = mem_address.range(1, 0);
            ap_uint<5>  shift = ((ap_uint<5>)boff) << 3;
            ap_uint<32> old   = dmem[widx];
            ap_uint<32> mask, data;
            if (mem_write_size == 0) {          // SB
                mask = ((ap_uint<32>)0xFF) << shift;
                data = ((ap_uint<32>)mem_write_data.range(7, 0)) << shift;
            } else if (mem_write_size == 1) {   // SH
                mask = ((ap_uint<32>)0xFFFF) << shift;
                data = ((ap_uint<32>)mem_write_data.range(15, 0)) << shift;
            } else {                            // SW
                mask = 0xFFFFFFFF;
                data = mem_write_data;
            }
            dmem[widx] = (old & ~mask) | (data & mask);
        }

        // ---------- 6. WRITEBACK ----------
        if (write_enable) {
            regs[rd_index] = rd_value;
        }

        // ---------- 7. NEXT PC ----------
        ap_uint<32> next_pc = branch_taken ? branch_target : (ap_uint<32>)(pc + 4);
        if (next_pc == pc) {                    // jump to itself = program finished
            halted = 1;
        }
        pc = next_pc;
        count++;
        n++;
    }

    // ---------------- 3. Debug outputs for VIO ----------------
    dbg_pc     = pc;
    dbg_instr  = imem[(pc >> 2) & (IMEM_WORDS - 1)];   // the next instruction
    dbg_reg    = regs[reg_sel];
    dbg_count  = count;
    dbg_halted = halted;
}
