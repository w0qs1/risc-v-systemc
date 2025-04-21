#include <systemc.h>
#include <iomanip>
#include <vector>
#include "config.h"
#include "gpio.cpp"
#include "systick.cpp"
#include "application.h"

using namespace std;

SC_MODULE(RV32I) {
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_in<sc_uint<32>> interrupt;
    sc_out<sc_uint<32>> interrupt_flag;
    sc_out<bool> halt;
    sc_out<sc_uint<64>> run_cycles;
    sc_out<sc_uint<64>> wfi_cycles;
    
    bool wfi_flag;

    GPIO* gpio0;
    GPIO* gpio1;
    SysTick* systick;

    // sc_signal<sc_uint<32>> interrupt_flag;
    sc_signal<sc_uint<32>> interrupt_sig;
    bool interrupt_pending;
    sc_signal<sc_uint<32>> return_pc;
    sc_signal<sc_uint<32>> pc;
    sc_signal<sc_uint<32>> registers[32];
    sc_signal<sc_uint<32>> instruction_memory[INSTR_MEM_SIZE];
    sc_signal<sc_uint<8>> data_memory[DATA_MEM_SIZE];
    sc_signal<sc_uint<32>> uepc;

    // GPIO signals:
    sc_signal<sc_uint<2>> gpio0_r_sel;
    sc_signal<bool> gpio0_s_write;
    sc_signal<bool> gpio0_s_read;
    sc_signal<sc_uint<32>> gpio0_data_in;
    sc_signal<sc_uint<32>> gpio0_data_out;

    sc_inout_rv<32> gpio0_inout;

    sc_signal<sc_uint<2>> gpio1_r_sel;
    sc_signal<bool> gpio1_s_write;
    sc_signal<bool> gpio1_s_read;
    sc_signal<sc_uint<32>> gpio1_data_in;
    sc_signal<sc_uint<32>> gpio1_data_out;

    sc_inout_rv<32> gpio1_inout;

    // SysTick Signals:
    sc_signal<bool> systick_int_enable;
    sc_signal<sc_uint<2>> systick_r_sel;
    sc_signal<bool> systick_s_write;
    sc_signal<bool> systick_s_read;
    sc_signal<sc_uint<32>> systick_rdata;
    sc_signal<sc_uint<32>> systick_wdata;
    sc_signal<bool> systick_irq;

    sc_uint<7> opcode;
    sc_uint<5> rd;
    sc_uint<3> funct3;
    sc_uint<5> rs1;
    sc_uint<5> rs2;
    sc_uint<7> funct7;
    sc_uint<5> shamt;

    // Immediate values are signed
    sc_int<12> imm_i;
    sc_int<12> imm_s;
    sc_int<12> imm_b;
    sc_int<20> imm_j;
    sc_uint<20> imm_u;

    // For memory operations
    sc_uint<32> address;
    sc_signal<bool> mem_accessed;

	sc_int<32> sign_extend_12(sc_int<12> num) {
        if (num[11]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 12) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }

    sc_int<32> sign_extend_8(sc_int<8> num) {
        if (num[7]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 8) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }

    sc_int<32> sign_extend_16(sc_int<16> num) {
        if (num[15]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 16) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }

    sc_int<32> sign_extend_20(sc_int<20> num) {
        if (num[19]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 20) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }

	sc_uint<32> zero_extend_12(sc_uint<12> num) {
		return num;
	}

	sc_uint<32> zero_extend_8(sc_uint<8> num) {
		return num;
	}

	sc_uint<32> zero_extend_16(sc_uint<16> num) {
		return num;
	}

    void handle_r_type(sc_uint<32> instr) {
        rd      = (instr >> 7) & 0x1F;
        funct3  = (instr >> 12) & 0x07;
        rs1     = (instr >> 15) & 0x1F;
        rs2     = (instr >> 20) & 0x1F;
        funct7  = (instr >> 25) & 0x7F;

        if(funct7 == 0x00 && funct3 == 0x0) {           // ADD
            cout << "ADD x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() + registers[rs2].read());
        } else if (funct7 == 0x20 && funct3 == 0x0) {   // SUB
            cout << "SUB x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() - registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x7) {   // AND
            cout << "AND x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() & registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x6) {   // OR
            cout << "OR x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() | registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x1) {   //SLL
            cout << "SLL x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() << registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x2) {   // SLT
            cout << "SLT x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) ((int32_t) registers[rs1].read() < (int32_t) registers[rs2].read()) ? 0x00000001 : 0x00000000);
        } else if (funct7 == 0x00 && funct3 == 0x3) {   // SLTU
            cout << "SLTU x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) ((uint32_t) registers[rs1].read() < (uint32_t) registers[rs2].read()) ? 0x00000001 : 0x00000000);
        } else if (funct7 == 0x00 && funct3 == 0x4) {   // XOR
            cout << "XOR x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() ^ registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x5) {	// SRL
            cout << "SRL x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) (uint32_t) registers[rs1].read() >> ((uint32_t) registers[rs2].read() & 0x1F));
        } else if (funct7 == 0x20 && funct3 == 0x5) {	// SRA
            cout << "SRA x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) (int32_t) registers[rs1].read() >> ((int32_t) registers[rs2].read() & 0x1F));
        }
    }

    void handle_i_type(sc_uint<32> instr) {
        rd      = (instr >> 7) & 0x1F;
        funct3  = (instr >> 12) & 0x07;
        rs1     = (instr >> 15) & 0x1F;
        shamt 	= (instr >> 20) & 0x1F;
        funct7 	= (instr >> 25) & 0x7F;
        //imm_i   = (instr & (1 << 31)) ? 0xFFFFF000 : 0x00000000;
        imm_i   = sign_extend_12((instr >> 20) & 0xFFF);
        // cout << dec << rd << hex << ", " << funct3 << ", "  << dec << rs1 << hex << ", "  << shamt << ", "  << funct7 << ", "  << dec << imm_i << hex << endl;
        sc_uint<32> read_data;

        if(funct3 == 0x0) {                             // ADDI
            cout << "ADDI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() + imm_i);
        } else if (funct3 == 0x5 && funct7 == 0x00) {	// SRLI
            cout << "SRLI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>)(uint32_t) registers[rs1].read() >> shamt);
        } else if (funct3 == 0x5 && funct7 == 0x20) {	// SRAI
            cout << "SARI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>)(int32_t) registers[rs1].read() >> shamt);
        } else if (opcode == 0x03 && funct3 == 0x0) {	// LB
            cout << "LB x" << dec << rd << hex << ", " << imm_i << "(x" << dec << dec << rs1 << hex << hex << ")" << endl;
            address = registers[rs1].read() + sign_extend_12(imm_i);
            if (address < DATA_MEM_END) {
                read_data = data_memory[address].read();
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel.write((address - GPIO0_BASE) / 4);
                gpio0_s_read = true;
                read_data = gpio0_data_out.read() & 0xFF;
                wait();
                gpio0_s_read = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel.write((address - GPIO1_BASE) / 4);
                gpio1_s_read = true;
                read_data = gpio1_data_out.read() & 0xFF;
                wait();
                gpio1_s_read = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_read = true;
                read_data = systick_rdata.read() & 0xFF;
                wait();
                systick_s_read = false;
            }
            registers[rd].write((sc_uint<32>) sign_extend_8((sc_int<8>) read_data));
        } else if (opcode == 0x03 && funct3 == 0x1) {	// LH
            cout << "LH x" << dec << rd << hex << ", " << imm_i << "(x" << dec << dec << rs1 << hex << hex << ")" << endl;
            address = registers[rs1].read() + sign_extend_12(imm_i);
            if (address < DATA_MEM_END) {
                read_data = ((data_memory[address + 1].read() << 8) | (data_memory[address].read())) & 0xFFFF;
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel.write((address - GPIO0_BASE) / 4);
                gpio0_s_read = true;
                read_data = gpio0_data_out.read() & 0xFFFF;
                wait();
                gpio0_s_read = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel.write((address - GPIO1_BASE) / 4);
                gpio1_s_read = true;
                read_data = gpio1_data_out.read() & 0xFFFF;
                wait();
                gpio1_s_read = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_read = true;
                read_data = systick_rdata.read() & 0xFFFF;
                wait();
                systick_s_read = false;
            }
            registers[rd].write((sc_uint<32>) sign_extend_16((sc_int<16>) read_data));
        } else if (opcode == 0x03 && funct3 == 0x2) {	// LW
            cout << "LW x" << dec << rd << hex << ", " << dec << imm_i << hex << "(x" << dec << rs1 << hex << ")" << endl;
            address = registers[rs1].read() + sign_extend_12(imm_i);
            if (address < DATA_MEM_END) {
                read_data = (sc_uint<32>) (data_memory[address + 3].read() << 24) | (data_memory[address + 2].read() << 16) | (data_memory[address + 1].read() << 8) | data_memory[address].read();
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel.write((address - GPIO0_BASE) / 4);
                gpio0_s_read = true;
                read_data = gpio0_data_out.read();
                wait();
                gpio0_s_read = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel.write((address - GPIO1_BASE) / 4);
                gpio1_s_read = true;
                read_data = gpio1_data_out.read();
                wait();
                gpio1_s_read = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_read = true;
                read_data = systick_rdata.read();
                wait();
                systick_s_read = false;
            }
            registers[rd].write((sc_uint<32>) read_data);
        } else if (opcode == 0x03 && funct3 == 0x4) {	// LBU
            cout << "LBU x" << dec << rd << hex << ", " << dec << imm_i << hex << "(x" << dec << rs1 << hex << ")" << endl;
            sc_uint<32> address = registers[rs1].read() + imm_i;
            registers[rd].write((sc_uint<32>)zero_extend_8(data_memory[address].read()));
        } else if (opcode == 0x03 && funct3 == 0x5) {	// LHU
            cout << "LH x" << dec << rd << hex << ", " << dec << imm_i << hex << "(x" << dec << rs1 << hex << ")" << endl;
            sc_uint<32> address = registers[rs1].read() + imm_i;
            registers[rd].write((sc_uint<32>) zero_extend_16((data_memory[address + 1].read() << 8) | (data_memory[address].read())));
        } else if (funct3 == 0x7) {                     // ANDI
            cout << "ANDI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() & imm_i);
        } else if (funct3 == 0x6) {                     // ORI
            cout << "ORI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() | imm_i);
        } else if (funct3 == 0x2) {						// SLTI
            cout << "SLTI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) (int32_t) registers[rs1].read() < (int32_t) imm_i);
        } else if (funct3 == 0x3) {						// SLTIU
            cout << "SLTIU x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) (uint32_t) registers[rs1].read() < (uint32_t) imm_i);
        } else if (funct3 == 0x4) {						// XORI
            cout << "XORI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() ^ imm_i);
        } else if (funct3 == 0x1) {						// SLLI
            cout << "SLLI x" << dec << rd << hex << ", x" << dec << rs1 << hex << ", " << dec << imm_i << hex << endl;
            registers[rd].write((sc_uint<32>) (uint32_t) registers[rs1].read() << shamt);
        }
    }

    void handle_s_type(sc_uint<32> instr) {
        funct3  = (instr >> 12) & 0x07;
        rs1     = (instr >> 15) & 0x1F;
        rs2     = (instr >> 20) & 0x1F;
        // imm_s   = instr >> 18;
        imm_s   = ((instr >> 25) << 5) | ((instr >> 7) & 0x1F);
        address = registers[rs1].read() + sign_extend_12(imm_s);

        if (funct3 == 0x0) {		// SB
            cout << "SB x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            if (address < DATA_MEM_END) {
                data_memory[address] = registers[rs2].read() & 0x000000FF;
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel = (address - GPIO0_BASE) / 4;
                gpio0_s_write = true;
                gpio0_data_in.write(registers[rs2].read() & 0x000000FF);
                wait();
                gpio0_s_write = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel = (address - GPIO1_BASE) / 4;
                gpio1_s_write = true;
                gpio1_data_in.write(registers[rs2].read() & 0x000000FF);
                wait();
                gpio1_s_write = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_write = true;
                systick_wdata.write(registers[rs2].read() & 0x000000FF);
                wait();
                systick_s_write = false;
            }
        } else if (funct3 == 0x1) {	// SH
            cout << "SH x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            if (address < DATA_MEM_END) {
                data_memory[address] = registers[rs2].read() & 0x000000FF;
                data_memory[address + 1] = (registers[rs2].read() & 0x0000FF00) >> 8;
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel = (address - GPIO0_BASE) / 4;
                gpio0_s_write = true;
                gpio0_data_in.write(registers[rs2].read() & 0x0000FFFF);
                wait();
                gpio0_s_write = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel = (address - GPIO1_BASE) / 4;
                gpio1_s_write = true;
                gpio1_data_in.write(registers[rs2].read() & 0x0000FFFF);
                wait();
                gpio1_s_write = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_write = true;
                systick_wdata.write(registers[rs2].read() & 0x0000FFFF);
                wait();
                systick_s_write = false;
            }
        } else if (funct3 == 0x2) {	// SW
            cout << "SW x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            if (address < DATA_MEM_END) {
                data_memory[address] = registers[rs2].read() & 0x000000FF;
                data_memory[address + 1] = (registers[rs2].read() & 0x0000FF00) >> 8;
                data_memory[address + 2] = (registers[rs2].read() & 0x00FF0000) >> 16;
                data_memory[address + 3] = (registers[rs2].read() & 0xFF000000) >> 24;
            } else if (address >= GPIO0_BASE && address < GPIO0_END) {
                gpio0_r_sel = (address - GPIO0_BASE) / 4;
                gpio0_s_write = true;
                gpio0_data_in.write(registers[rs2].read());
                wait();
                gpio0_s_write = false;
            } else if (address >= GPIO1_BASE && address < GPIO1_END) {
                gpio1_r_sel = (address - GPIO1_BASE) / 4;
                gpio1_s_write = true;
                gpio1_data_in.write(registers[rs2].read());
                wait();
                gpio1_s_write = false;
            } else if (address >= SYSTICK_BASE && address < SYSTICK_END) {
                systick_r_sel.write((address - SYSTICK_BASE) / 4);
                systick_s_write = true;
                systick_wdata.write(registers[rs2].read());
                wait();
                systick_s_write = false;
            }
        }
    }

    void handle_u_type_lui(sc_uint<32> instr) {
        cout << "LUI x" << dec << rd << hex << ", " << ((instr && 0xFFFFF000) >> 12) << endl;
        rd = (instr >> 7) & 0x1F;

        registers[rd].write((sc_uint<32>) instr & 0xFFFFF000);
    }
    void handle_u_type_auipc(sc_uint<32> instr) {
        cout << "AUPIC x" << dec << rd << hex << ", " << ((instr && 0xFFFFF000) >> 12) << endl;
        rd = (instr >> 7) & 0x1F;

        registers[rd].write((sc_uint<32>) pc.read() + (instr & 0xFFFFF000));
    }

    void handle_j_type_jal(sc_uint<32> instr) {
        rd = (instr >> 7) & 0x1F;
        sc_uint<1> imm_j_1 = (instr >> 31) & 0x1;
        sc_uint<8> imm_j_2 = (instr >> 12) & 0xFF;
        sc_uint<1> imm_j_3 = (instr >> 20) & 0x1;
        sc_uint<10> imm_j_4 = (instr >> 21) & 0x3FF;

        imm_j = sign_extend_20((imm_j_1 << 19) | (imm_j_2 << 11) | (imm_j_3 << 10) | imm_j_4);
        imm_j <<= 1;
        cout << "JAL x" << dec << rd << hex << ", 0x" << setw(8) << setfill('0') << imm_j << endl;

        registers[rd].write((sc_uint<32>) pc.read() + 4);
        // -4 because pc gets incremented
        pc.write(pc.read() + imm_j - 4);
    }

    void handle_j_type_jalr(sc_uint<32> instr) {
        rd = (instr >> 7) & 0x1F;
        rs1 = (instr >> 15) & 0x1F;
        imm_j = sign_extend_12((instr >> 20) & 0xFFF);
        imm_j <<= 1;
        cout << "JALR x" << dec << rd << hex << ", x" << dec << rs1 << hex << "(" << imm_j << ")" << endl;

        registers[rd].write((sc_uint<32>)registers[rs1].read() + imm_j);
        pc.write((sc_uint<32>)registers[rs1].read() + imm_j - 4);
    }

    void handle_b_type(sc_uint<32> instr) {
        funct3  = (instr >> 12) & 0x07;
        rs1 = (instr >> 15) & 0x1F;
        rs2 = (instr >> 20) & 0x1F;
        sc_uint<1> imm_b_1 = (instr >> 31) & 0x1;
        sc_uint<1> imm_b_2 = (instr >> 7) & 0x1;
        sc_uint<6> imm_b_3 = (instr >> 25) & 0x3F;
        sc_uint<4> imm_b_4 = (instr >> 8) & 0xF;

        imm_b = sign_extend_12((imm_b_1 << 11) | (imm_b_2 << 10) | (imm_b_3 << 4) | imm_b_4);
        imm_b <<= 1;

        if (funct3 == 0x0) {            // BEQ
            cout << "BEQ x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() == registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x01) {    // BNE
            cout << "BNE x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if (registers[rs1].read() != registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                // wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x04) {    // BLT
            cout << "BLT x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if ((int32_t) registers[rs1].read() < (int32_t) registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x05) {    // BGE
            cout << "BGE x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if ((int32_t) registers[rs1].read() >= (int32_t) registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x06) {    // BLTU
            cout << "BLTU x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() < registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x07) {    // BGEU
            cout << "BGEU x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() >= registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 4);
                cout << hex << " | Branch Taken, New PC: 0x" << setw(8) << setfill('0') << pc.read() + 4 << endl;
                wait();
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        }
    }

    void handle_sys_type(sc_uint<32> instr) {
        if(instr == 0x00000073) {           // ecall
            halt.write(true);
            cout << "System Call: Halted!" << endl;
        } else if (instr == 0x00200073) {   // uret
            // Set the pc to the return address (of user code) stored in uepc register
            cout << "URET" << endl;
            cout << "Interrupt Complete!" << endl;
            interrupt_flag.write(0);
            pc.write(uepc.read() - 4);
        } else if (instr == 0x10500073) {   // wfi
            // Wait for interrupt
            cout << "WFI" << endl;
            wfi_flag = true;
        }
    }

    void handle_invalid_instruction(sc_uint<32> instr) {
        cout << hex << "Invalid Instruction: 0x" << instr.to_uint() << " at PC: 0x" << pc.read() << endl;
        halt.write(true);
    }

    void execute(sc_uint<32> instr) {
        cout << hex << "PC: 0x" << setw(8) << setfill('0') << (uint32_t) pc.read() << " | Instruction: " << setw(8) << setfill('0') << (uint32_t) instr << " | ";
        opcode = instr & 0x7F;
        switch(opcode) {
            case 0x33: // R-type
                handle_r_type(instr);
                break;

            case 0x03:  // I-type
            case 0x13:
                handle_i_type(instr);
                break;

            case 0x23:  // S-type
                handle_s_type(instr);
				break;

            case 0x37:  // U-type (LUI)
                handle_u_type_lui(instr);
                break;

            case 0x17:  // U-type (AUIPC)
                handle_u_type_auipc(instr);
                break;

            case 0x6F:  // J-type (JAL)
                handle_j_type_jal(instr);
                break;

            case 0x67:  // J-type (JALR)
                handle_j_type_jalr(instr);
                break;

            case 0x63:  // B-type
                handle_b_type(instr);
                break;

            case 0x73:  // e-call, uret, wfi;
                handle_sys_type(instr);
                break;

            default:
                handle_invalid_instruction(instr);
                break;
    	}
        registers[0].write(0x00000000UL);
    }

    void reset_registers(void) {
        for(uint8_t i = 0; i < 32; i++) {
            registers[i].write(0x00000000UL);
        }
    }

    // Only for debugging
    void set_instruction_mem(uint32_t *program, uint16_t length) {
        for(uint16_t i = 0; i < length; i++) {
            instruction_memory[i].write(*(program + i));
        }
    }

    void set_data_mem(uint8_t *data, uint16_t length) {
        for(uint16_t i = 0; i < length; i++) {
            data_memory[i].write(*(data+i));
        }
    }

    void read_registers(void) {
        for(uint8_t i = 0; i < 8; i++) {
            // cout << "x" << i << ": " << registers[i].read() << endl;
            cout << "x" << setw(2) << setfill('0') << (int)i << ": " 
             << hex << setw(8) << setfill('0') << registers[i].read() 
             << dec << endl;
        }
    }

    void fetch_decode(void) {
        // uint32_t test_program[9] = {0x00500293, 0x00000313, 0x00032383, 0x00538393, 0x00732023, 0xfff28293, 0x00430313, 0xfe0296e3, 0x00000073};
        // uint32_t test_program[13] = {0x0100006f, 0x0040006f, 0x00150513, 0x00200073, 0x00400293, 0x20000313, 0x00032383, 0x00538393, 0x00732023, 0xfff28293, 0x00430313, 0xfe0296e3, 0x00000073};
        extern uint32_t test_program[32];
        extern uint8_t test_data[20];
        while(true) {
            wait();
            if(nreset.read() == 0) {
                cout << "Processor Reset!" << endl;
                // Reset handler at location 0x00
                pc.write(0);
                halt.write(false);
                mem_accessed.write(false);
                interrupt_flag.write(0);
                reset_registers();
                set_instruction_mem(test_program, 32);
                set_data_mem(test_data, 20);
                continue;
            } else if ((sc_uint<32>) interrupt && (sc_uint<32>) !interrupt_flag.read()) {
                // interrupt flag is read to prevent multiple triggers
                // cout << "Inside Interrupt Logic!" << endl;
                sc_uint<8> interrupt_num = 0;
                for(uint8_t i = 0; i < 32; i++) {
                    if ((sc_uint<32>) interrupt & (1 << i)) {
                        interrupt_num = i;
                        break;
                    }
                }
                cout << "Interrupt " << dec << interrupt_num << " Detected at " << sc_time_stamp() << endl;
                wfi_flag = false;

                // set the flag corresponding to the interrupt
                interrupt_flag.write(1 << interrupt_num);
                interrupt_pending = false;

                // store pc to interrupt return register for returning after completing isr
                uepc.write(pc.read());
                // read_registers();
                
                // Interrupt handler starts at location 0x04 (each interrupt is given 4B)
                pc.write(4 + (4 * interrupt_num));
                continue;
            } else if (!wfi_flag) {
                // read_registers();
                // cout << "Sig: " << interrupt_sig.read() << " Flag: " << interrupt_flag.read() << " | ";
                execute(instruction_memory[pc.read() / 4].read());
                wait(); // wait for changes to take place
                pc.write(pc.read() + 4);    // word size
            }
        }
    }

    // void combine_interrupts(void) {
    //     sc_uint<32> irq_val = interrupt.read();
    //     // cout << "irq before: " << irq_val << endl;
    //     // irq_val[0] |= systick_irq.read();  // Bit 0 for systick
    //     interrupt_sig.write(irq_val);
    //     // cout << "\nSig: " << interrupt_sig.read() << " Flag: " << interrupt_flag.read() << endl;
    //     if (interrupt_sig.read() && !interrupt_pending) {
    //         interrupt_pending = true;
    //     }
    // }

    // void turn_off_interrupt(void) {
    //     systick_int_enable.write(!interrupt_flag.read()[0]);
    // }

    void count_cycles(void) {
        if(wfi_flag == true) {
            wfi_cycles.write((sc_uint<64>) wfi_cycles.read() + 1);
        } else {
            run_cycles.write((sc_uint<64>) run_cycles.read() + 1);
        }
    }

    SC_CTOR(RV32I) {
        // run_cycles.write(0);
        // wfi_cycles.write(0);

        SC_THREAD(fetch_decode);
        sensitive << clk.pos() << nreset << interrupt;

        SC_METHOD(count_cycles);
        sensitive << clk.pos();

        // SC_METHOD(combine_interrupts);
        // sensitive << interrupt << systick_irq;

        // SC_METHOD(turn_off_interrupt);
        // sensitive << interrupt_flag;

        gpio0 = new GPIO("gpio0");
        gpio0->clk(clk);
        gpio0->nreset(nreset);
        gpio0->r_sel(gpio0_r_sel);
        gpio0->s_write(gpio0_s_write);
        gpio0->s_read(gpio0_s_read);
        gpio0->data_out(gpio0_data_out);
        gpio0->data_in(gpio0_data_in);
        gpio0->gpio_inout(gpio0_inout);

        gpio1 = new GPIO("gpio1");
        gpio1->clk(clk);
        gpio1->nreset(nreset);
        gpio1->r_sel(gpio1_r_sel);
        gpio1->s_write(gpio1_s_write);
        gpio1->s_read(gpio1_s_read);
        gpio1->data_out(gpio1_data_out);
        gpio1->data_in(gpio1_data_in);
        gpio1->gpio_inout(gpio1_inout);

        systick = new SysTick("systick");
        systick->clk(clk);
        systick->nreset(nreset);
        systick->int_enable(systick_int_enable);
        systick->r_sel(systick_r_sel);
        systick->s_write(systick_s_write);
        systick->s_read(systick_s_read);
        systick->wdata(systick_wdata);
        systick->rdata(systick_rdata);
        systick->irq(systick_irq);
    }
};
