# RV32I RISC-V Processor

I built this 32-bit RV32I multi-cycle processor in C++ using Vitis HLS and ran it on a Digilent Zybo board (Zynq-7010) at 100 MHz.

The processor supports all 40 RV32I base instructions. The register file, instruction ROM, and data RAM are mapped to BRAM. I used Vivado VIO to step through programs and check register and memory values on the board.

## How I tested it

I tested each part first, then ran 2 million random vectors against a separate reference model. I also ran 500 random programs and compared the results with RARS. These tests finished with 0 mismatches.

To check that the testbenches could catch real problems, I added bugs on purpose and ran mutation testing. I also ran C/RTL co-simulation on the generated RTL.

## Files

- `riscv_cpu.cpp` and `riscv_cpu.h` - processor design
- `riscv_soc.cpp` - SoC design
- `riscv_fpga.cpp` - FPGA version
- `riscv_soc_tb.cpp` - testbench
- `program.h` - test program

## Tools

Vitis HLS 2022.1, Vivado, C++, and RARS
