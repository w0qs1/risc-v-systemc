#include <systemc.h>
#include <iomanip>
#include <vector>
#include "config.h"

using namespace std;

SC_MODULE(RV32I) {
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_out<bool> halt;

    sc_signal<sc_uint<32>> pc;
    sc_signal<sc_uint<32>> registers[32];
    sc_signal<sc_uint<32>> instruction_memory[INSTR_MEM_SIZE];
    sc_signal<sc_uint<8>> data_memory[DATA_MEM_SIZE];

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

	sc_int<32> sign_extend_12(sc_int<12> num) {
        if (num[11]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 12) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }

    sc_int<32> sign_extend_8(sc_uint<8> num) {
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
            cout << "ADD x" << dec << rd << hex << ", " << dec << rs1 << hex << ", " << dec << rs2 << hex << endl;
            registers[rd].write((sc_uint<32>) registers[rs1].read() + registers[rs2].read());
        } else if (funct7 == 0x20 && funct3 == 0x0) {   // SUB
            registers[rd].write((sc_uint<32>) registers[rs1].read() - registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x7) {   // AND
            registers[rd].write((sc_uint<32>) registers[rs1].read() & registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x6) {   // OR
            registers[rd].write((sc_uint<32>) registers[rs1].read() | registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x1) {   //SLL
            registers[rd].write((sc_uint<32>) registers[rs1].read() << registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x2) {   // SLT
            registers[rd].write((sc_uint<32>) ((int32_t) registers[rs1].read() < (int32_t) registers[rs2].read()) ? 0x00000001 : 0x00000000);
        } else if (funct7 == 0x00 && funct3 == 0x3) {   // SLTU
            registers[rd].write((sc_uint<32>) ((uint32_t) registers[rs1].read() < (uint32_t) registers[rs2].read()) ? 0x00000001 : 0x00000000);
        } else if (funct7 == 0x00 && funct3 == 0x4) {   // XOR
            registers[rd].write((sc_uint<32>) registers[rs1].read() ^ registers[rs2].read());
        } else if (funct7 == 0x00 && funct3 == 0x5) {	// SRL
            registers[rd].write((sc_uint<32>) (uint32_t) registers[rs1].read() >> ((uint32_t) registers[rs2].read() & 0x1F));
        } else if (funct7 == 0x20 && funct3 == 0x5) {	// SRA
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
            registers[rd].write((sc_uint<32>) sign_extend_8(data_memory[registers[rs1].read() + sign_extend_12(imm_i)].read())); 
        } else if (opcode == 0x03 && funct3 == 0x1) {	// LH
            cout << "LH x" << dec << rd << hex << ", " << imm_i << "(x" << dec << dec << rs1 << hex << hex << ")" << endl;
            sc_uint<32> address = registers[rs1].read() + imm_i;
            registers[rd].write((sc_uint<32>) sign_extend_16((data_memory[address + 1].read() << 8) | (data_memory[address].read())));
        } else if (opcode == 0x03 && funct3 == 0x2) {	// LW
            cout << "LW x" << dec << rd << hex << ", " << dec << imm_i << hex << "(x" << dec << rs1 << hex << ")" << endl;
            sc_uint<32> address = registers[rs1].read() + imm_i;
            registers[rd].write((sc_uint<32>) (data_memory[address + 3].read() << 24) | (data_memory[address + 2].read() << 16) | (data_memory[address + 1].read() << 8) | data_memory[address].read());
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

        if (funct3 == 0x0) {		// SB
            cout << "SB x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s)] = registers[rs2].read() & 0x000000FF;
        } else if (funct3 == 0x1) {	// SH
            cout << "SH x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s)] = registers[rs2].read() & 0x000000FF;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s) + 1] = (registers[rs2].read() & 0x0000FF00) >> 8;
        } else if (funct3 == 0x2) {	// SW
            cout << "SW x" << dec << rs2 << hex << ", " << dec << imm_s << "(x" << dec << rs1 << hex << ")" << endl;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s)] = registers[rs2].read() & 0x000000FF;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s) + 1] = (registers[rs2].read() & 0x0000FF00) >> 8;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s) + 2] = (registers[rs2].read() & 0x00FF0000) >> 16;
            data_memory[registers[rs1].read() + sign_extend_12(imm_s) + 3] = (registers[rs2].read() & 0xFF000000) >> 24;
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
        cout << "JAL x" << dec << rd << hex << ", " << dec << imm_j << endl;

        registers[rd].write((sc_uint<32>) pc.read() + 1);
        // -1 because pc gets incremented
        pc.write(pc.read() + imm_j - 1);
    }

    void handle_j_type_jalr(sc_uint<32> instr) {
        rd = (instr >> 7) & 0x1F;
        rs1 = (instr >> 15) & 0x1F;
        imm_j = sign_extend_12((instr >> 20) & 0xFFF);
        cout << "JALR x" << dec << rd << hex << ", x" << dec << rs1 << hex << "(" << dec << imm_j << ")" << endl;

        registers[rd].write((sc_uint<32>)registers[rs1].read() + imm_j);
        pc.write((sc_uint<32>) pc.read() + imm_j + registers[rs1].read() - 1);
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
        imm_b >>= 1;

        if (funct3 == 0x0) {            // BEQ
            cout << "BEQ x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() == registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x01) {    // BNE
            cout << "BNE x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if (registers[rs1].read() != registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x04) {    // BLT
            cout << "BLT x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if ((int32_t) registers[rs1].read() < (int32_t) registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x05) {    // BGE
            cout << "BGE x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if ((int32_t) registers[rs1].read() >= (int32_t) registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x06) {    // BLTU
            cout << "BLTU x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() < registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
        } else if (funct3 == 0x07) {    // BGEU
            cout << "BGEU x" << dec << rs1 << hex << ", x" << dec << rs2 << hex << ", " << dec << imm_b;
            if(registers[rs1].read() >= registers[rs2].read()) {
                pc.write((sc_uint<32>) pc.read() + imm_b - 1);
                wait();
                cout << hex << " | Branch Taken, New PC: " << pc.read() + 1 << endl;
            } else {
                cout << hex << " | Branch NOT Taken" << endl;
            }
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

            case 0x73:  // e-call;
                halt.write(true);
                cout << "System Call: Halted!" << endl;
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
        uint32_t test_program[10] = {0x00500293, 0x00000313, 0x00032383, 0x00538393, 0x00732023, 0xfff28293, 0x00430313, 0xfe0296e3, 0x00000073};
        uint8_t test_data[20] = {0x22, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00};
        while(true) {
            wait();
            if(nreset.read() == 0) {
                cout << "Processor Reset!" << endl;
                pc.write(0);
                halt.write(false);
                reset_registers();
                set_instruction_mem(test_program, 9);
                set_data_mem(test_data, 20);
                continue;
            }
            // read_registers();
            execute(instruction_memory[pc.read()].read());
            wait(); // wait for changes to take place
            pc.write(pc.read() + 1);    // word size
        }
    }

    SC_CTOR(RV32I) {
        SC_THREAD(fetch_decode);
        sensitive << clk.pos() << nreset;
    }
};
