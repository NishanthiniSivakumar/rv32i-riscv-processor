#include <ap_int.h>

void riscv_cpu(
    ap_uint<32> instruction,
    ap_uint<32> rs1_value,
    ap_uint<32> rs2_value,
	 ap_uint<32> pc,
    ap_uint<32> &rd_value,
    ap_uint<5>  &rd_index,
    ap_uint<1>  &write_enable,

	 // New memory interface outputs
	    ap_uint<32> &mem_address,
	    ap_uint<32> &mem_write_data,
	    ap_uint<1>  &mem_write_enable,
	    ap_uint<2>  &mem_write_size,

		ap_uint<1>  &branch_taken,
		ap_uint<32> &branch_target,

		// Memory read request outputs
		ap_uint<32> &mem_read_address,
		ap_uint<1>  &mem_read_enable,
		ap_uint<2>  &mem_read_size,

		// Data returned by memory
		ap_uint<32> mem_read_data   // full aligned 32-bit word at (mem_read_address & ~3)
)
{
    // ----------------------------------
    // 1. Decode instruction fields
    // ----------------------------------
    ap_uint<7> opcode = instruction.range(6, 0);
    ap_uint<5> rd     = instruction.range(11, 7);
    ap_uint<3> funct3 = instruction.range(14, 12);
    ap_uint<5> rs2    = instruction.range(24, 20);
    ap_uint<7> funct7 = instruction.range(31, 25);

    ap_int<12> imm12 = (ap_int<12>)instruction.range(31, 20);
    ap_int<32> imm32 = imm12;

    // ----------------------------------
    // 2. Default outputs
    // ----------------------------------
    rd_value = 0;
    rd_index = rd;
    write_enable = 0;
    mem_address = 0;
    mem_write_data = 0;
    mem_write_enable = 0;
    mem_write_size = 0;
    branch_taken = 0;
    branch_target = 0;
    mem_read_address = 0;
    mem_read_enable = 0;
    mem_read_size = 0;

    // ==================================
    // 3. R-TYPE INSTRUCTIONS
    // Opcode = 0x33
    // ==================================

    // ADD: rd = rs1 + rs2
    if (opcode == 0x33 &&
        funct3 == 0x0 &&
        funct7 == 0x00) {
        rd_value = rs1_value + rs2_value;
        write_enable = (rd != 0);
    }

    // SUB: rd = rs1 - rs2
    else if (opcode == 0x33 &&
             funct3 == 0x0 &&
             funct7 == 0x20) {
        rd_value = rs1_value - rs2_value;
        write_enable = (rd != 0);
    }

    // SLL: Shift Left Logical
    else if (opcode == 0x33 &&
             funct3 == 0x1 &&
             funct7 == 0x00) {
        ap_uint<5> shamt = rs2_value.range(4, 0);
        rd_value = rs1_value << shamt;
        write_enable = (rd != 0);
    }

    // SLT: Set Less Than (signed)
    else if (opcode == 0x33 &&
             funct3 == 0x2 &&
             funct7 == 0x00) {
        rd_value = ((ap_int<32>)rs1_value <
                    (ap_int<32>)rs2_value) ? 1 : 0;
        write_enable = (rd != 0);
    }

    // SLTU: Set Less Than (unsigned)
    else if (opcode == 0x33 &&
             funct3 == 0x3 &&
             funct7 == 0x00) {
        rd_value = (rs1_value < rs2_value) ? 1 : 0;
        write_enable = (rd != 0);
    }

    // XOR: rd = rs1 ^ rs2
    else if (opcode == 0x33 &&
             funct3 == 0x4 &&
             funct7 == 0x00) {
        rd_value = rs1_value ^ rs2_value;
        write_enable = (rd != 0);
    }

    // SRL: Shift Right Logical
    else if (opcode == 0x33 &&
             funct3 == 0x5 &&
             funct7 == 0x00) {
        ap_uint<5> shamt = rs2_value.range(4, 0);
        rd_value = rs1_value >> shamt;
        write_enable = (rd != 0);
    }

    // SRA: Shift Right Arithmetic
    else if (opcode == 0x33 &&
             funct3 == 0x5 &&
             funct7 == 0x20) {
        ap_uint<5> shamt = rs2_value.range(4, 0);
        rd_value = (ap_int<32>)rs1_value >> shamt;
        write_enable = (rd != 0);
    }

    // OR: rd = rs1 | rs2
    else if (opcode == 0x33 &&
             funct3 == 0x6 &&
             funct7 == 0x00) {
        rd_value = rs1_value | rs2_value;
        write_enable = (rd != 0);
    }

    // AND: rd = rs1 & rs2
    else if (opcode == 0x33 &&
             funct3 == 0x7 &&
             funct7 == 0x00) {
        rd_value = rs1_value & rs2_value;
        write_enable = (rd != 0);
    }

    // ==================================
    // 4. I-TYPE ALU INSTRUCTIONS
    // Opcode = 0x13
    // ==================================

    // ADDI: rd = rs1 + sign-extended immediate
    else if (opcode == 0x13 &&
             funct3 == 0x0) {
        rd_value = rs1_value + imm32;
        write_enable = (rd != 0);
    }

    // SLTI: Set Less Than Immediate (signed)
    else if (opcode == 0x13 &&
             funct3 == 0x2) {
        rd_value = ((ap_int<32>)rs1_value < imm32) ? 1 : 0;
        write_enable = (rd != 0);
    }

    // SLTIU: Set Less Than Immediate (unsigned)
    else if (opcode == 0x13 &&
             funct3 == 0x3) {
        rd_value = (rs1_value < (ap_uint<32>)imm32) ? 1 : 0;
        write_enable = (rd != 0);
    }

    // XORI: rd = rs1 ^ sign-extended immediate
    else if (opcode == 0x13 &&
             funct3 == 0x4) {
        rd_value = rs1_value ^ (ap_uint<32>)imm32;
        write_enable = (rd != 0);
    }

    // ORI: rd = rs1 | sign-extended immediate
    else if (opcode == 0x13 &&
             funct3 == 0x6) {
        rd_value = rs1_value | (ap_uint<32>)imm32;
        write_enable = (rd != 0);
    }

    // ANDI: rd = rs1 & sign-extended immediate
    else if (opcode == 0x13 &&
             funct3 == 0x7) {
        rd_value = rs1_value & (ap_uint<32>)imm32;
        write_enable = (rd != 0);
    }

    // SLLI: Shift Left Logical Immediate
    else if (opcode == 0x13 &&
             funct3 == 0x1 &&
             instruction.range(31, 25) == 0x00) {
        ap_uint<5> shamt = instruction.range(24, 20);
        rd_value = rs1_value << shamt;
        write_enable = (rd != 0);
    }

    // SRLI: Shift Right Logical Immediate
    else if (opcode == 0x13 &&
             funct3 == 0x5 &&
             instruction.range(31, 25) == 0x00) {
        ap_uint<5> shamt = instruction.range(24, 20);
        rd_value = rs1_value >> shamt;
        write_enable = (rd != 0);
    }

    // SRAI: Shift Right Arithmetic Immediate
    else if (opcode == 0x13 &&
             funct3 == 0x5 &&
             instruction.range(31, 25) == 0x20) {
        ap_uint<5> shamt = instruction.range(24, 20);
        rd_value = (ap_int<32>)rs1_value >> shamt;
        write_enable = (rd != 0);
    }

    // S-type: STORE

    else if (opcode == 0x23) {  // S-type: STORE

        ap_uint<12> store_imm;
        store_imm.range(11, 5) = instruction.range(31, 25);
        store_imm.range(4, 0)  = instruction.range(11, 7);

        ap_int<12> signed_store_imm = (ap_int<12>)store_imm;
        ap_int<32> store_offset = signed_store_imm;

        mem_address = rs1_value + store_offset;
        mem_write_data = rs2_value;

        if (funct3 == 0x0) {          // SB
            mem_write_enable = 1;
            mem_write_size = 0;
        }
        else if (funct3 == 0x1) {     // SH
            mem_write_enable = 1;
            mem_write_size = 1;
        }
        else if (funct3 == 0x2) {     // SW
            mem_write_enable = 1;
            mem_write_size = 2;
        }
    }

    // B-type: BRANCH

    else if (opcode == 0x63) {  // B-type: BRANCH

        ap_uint<13> branch_imm;

        branch_imm[12] = instruction[31];
        branch_imm[11] = instruction[7];
        branch_imm.range(10, 5) = instruction.range(30, 25);
        branch_imm.range(4, 1) = instruction.range(11, 8);
        branch_imm[0] = 0;

        ap_int<13> signed_branch_imm = (ap_int<13>)branch_imm;
        ap_int<32> branch_offset = signed_branch_imm;

        bool condition = false;

        if (funct3 == 0x0) {          // BEQ
            condition = (rs1_value == rs2_value);
        }
        else if (funct3 == 0x1) {     // BNE
            condition = (rs1_value != rs2_value);
        }
        else if (funct3 == 0x4) {     // BLT
            condition = ((ap_int<32>)rs1_value < (ap_int<32>)rs2_value);
        }
        else if (funct3 == 0x5) {     // BGE
            condition = ((ap_int<32>)rs1_value >= (ap_int<32>)rs2_value);
        }
        else if (funct3 == 0x6) {     // BLTU
            condition = (rs1_value < rs2_value);
        }
        else if (funct3 == 0x7) {     // BGEU
            condition = (rs1_value >= rs2_value);
        }

        if (funct3 == 0x0 || funct3 == 0x1 ||
            funct3 == 0x4 || funct3 == 0x5 ||
            funct3 == 0x6 || funct3 == 0x7) {

            branch_taken = condition;

            if (condition) {
                branch_target = pc + branch_offset;
            }
        }
    }

    // U-type instructions: LUI and AUIPC
    else if (opcode == 0x37) {
        // LUI: Load Upper Immediate
        rd_value = instruction.range(31, 12);
        rd_value = rd_value << 12;

        rd_index = rd;
        write_enable = (rd != 0);
    }
    else if (opcode == 0x17) {
        // AUIPC: Add Upper Immediate to PC
        ap_uint<32> upper_imm = instruction.range(31, 12);
        upper_imm = upper_imm << 12;

        rd_value = pc + upper_imm;

        rd_index = rd;
        write_enable = (rd != 0);
    }

    // J-type: JAL (Jump and Link)
    else if (opcode == 0x6F) {
        ap_uint<21> jal_imm;

        jal_imm[20] = instruction[31];
        jal_imm.range(19, 12) = instruction.range(19, 12);
        jal_imm[11] = instruction[20];
        jal_imm.range(10, 1) = instruction.range(30, 21);
        jal_imm[0] = 0;

        ap_int<21> signed_jal_imm = (ap_int<21>)jal_imm;
        ap_int<32> jal_offset = signed_jal_imm;

        // Save return address
        rd_value = pc + 4;
        rd_index = rd;
        write_enable = (rd != 0);

        // Jump to target address
        branch_taken = 1;
        branch_target = pc + jal_offset;
    }

    // I-type: Load instructions
    // Memory contract: mem_read_data is the full 32-bit word stored at the
    // aligned address (mem_read_address & ~3), little-endian.
    // This block selects the byte/halfword lane using address bits [1:0].
    else if (opcode == 0x03) {
        ap_uint<12> load_imm = instruction.range(31, 20);
        ap_int<12> signed_load_imm = (ap_int<12>)load_imm;
        ap_int<32> load_offset = signed_load_imm;

        ap_uint<32> load_addr = rs1_value + load_offset;
        ap_uint<2>  byte_off  = load_addr.range(1, 0);

        mem_read_address = load_addr;

        // Lane selection: shift the wanted byte/halfword down to bits [15:0]
        ap_uint<5>  lane_shift = ((ap_uint<5>)byte_off) << 3;   // 0, 8, 16, 24
        ap_uint<32> shifted = mem_read_data >> lane_shift;
        ap_uint<8>  byte_u  = shifted.range(7, 0);
        ap_uint<16> half_u  = shifted.range(15, 0);

        if (funct3 == 0x0) {            // LB: sign-extend byte
            mem_read_enable = 1;
            mem_read_size = 0;
            rd_value = (ap_int<8>)byte_u;
            write_enable = (rd != 0);
        }
        else if (funct3 == 0x1) {       // LH: sign-extend halfword
            mem_read_enable = 1;
            mem_read_size = 1;
            rd_value = (ap_int<16>)half_u;
            write_enable = (rd != 0);
        }
        else if (funct3 == 0x2) {       // LW: full word
            mem_read_enable = 1;
            mem_read_size = 2;
            rd_value = mem_read_data;
            write_enable = (rd != 0);
        }
        else if (funct3 == 0x4) {       // LBU: zero-extend byte
            mem_read_enable = 1;
            mem_read_size = 0;
            rd_value = byte_u;
            write_enable = (rd != 0);
        }
        else if (funct3 == 0x5) {       // LHU: zero-extend halfword
            mem_read_enable = 1;
            mem_read_size = 1;
            rd_value = half_u;
            write_enable = (rd != 0);
        }
        // funct3 = 3, 6, 7: illegal load -> no read, no write (defaults)
        // NOTE: misaligned LH/LHU (addr[0]=1) and LW (addr[1:0]!=0) are
        // not handled yet; policy [UNDECIDED].
    }

    // I-type: JALR (Jump and Link Register)
    else if (opcode == 0x67 && funct3 == 0x0) {
        ap_uint<12> jalr_imm = instruction.range(31, 20);
        ap_int<12> signed_jalr_imm = (ap_int<12>)jalr_imm;
        ap_int<32> jalr_offset = signed_jalr_imm;

        // Save return address
        rd_value = pc + 4;
        rd_index = rd;
        write_enable = (rd != 0);

        // Jump target = (rs1 + imm) with bit 0 cleared.
        // Uses rs1_value read before writeback, so rd == rs1 is safe.
        ap_uint<32> jalr_sum = rs1_value + jalr_offset;
        jalr_sum[0] = 0;
        branch_taken = 1;
        branch_target = jalr_sum;
    }

    // FENCE (opcode 0x0F): orders memory accesses. This CPU does one memory
    // access at a time and has no cache, so FENCE is a no-op (allowed by the
    // spec). All outputs stay at their defaults: no write, PC = PC + 4.
    else if (opcode == 0x0F) {
        // no operation
    }

    // ECALL (0x00000073) / EBREAK (0x00100073): request to the OS / debugger.
    // This CPU has no OS or trap system, so both STOP the program:
    // the "next PC" is set to the same PC, which the top level detects as halt.
    else if (instruction == 0x00000073 || instruction == 0x00100073) {
        branch_taken  = 1;
        branch_target = pc;
    }

    // Any other opcode: outputs stay at safe defaults
    // (no register write, no memory access, no branch).
}
