#include <systemc.h>
#include "rv32i.cpp"
//#include <tracing.h>
#include "config.h"

SC_MODULE(Testbench) {
    sc_signal<bool> clk, nreset, halt;
    sc_signal_rv<32> gpio0_inout, gpio1_inout;
    sc_signal<sc_uint<32>> interrupt;
    sc_signal<sc_uint<32>> interrupt_flag;
    sc_signal<sc_uint<64>> run_cycles;
    sc_signal<sc_uint<64>> wfi_cycles;

    RV32I *rv32i;

    sc_signal<sc_uint<64>> clock_count;
    sc_signal<bool> emu_systick_int;

    sc_uint<64> gpio10_count;
    sc_uint<64> gpio11_count;

    sc_signal<sc_uint<32>> internal_interrupts;

    void clk_gen() {
        while (true) {
            clk.write(false);
            wait(1, SC_NS);
            clk.write(true);
            clock_count = (sc_uint<64>) clock_count + 1;
            wait(1, SC_NS);
        }
    }

    void stim_proc() {
        nreset.write(false);
        gpio0_inout.write(sc_lv<32>("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"));
        gpio1_inout.write(sc_lv<32>("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"));
        wait(2, SC_NS);
        nreset.write(true);
        // wait(40, SC_NS);
        // internal_interrupts.write(0x000000002);
        // wait(10, SC_NS);
        // internal_interrupts.write(0x000000000);
        // wait(30, SC_NS);
        // gpio1_inout.write(sc_lv<32>("01010101010101010101010101010101"));
        // wait(30, SC_NS);
        // gpio1_inout.write(sc_lv<32>("10101010101010101010101010101010"));
        // wait(10, SC_NS);

        wait(STOP_TIME, SC_NS);
        cout << endl << "---------------------------------------" << endl;
        cout << "Generated Power and Energy Report:" << endl;
        cout << "---------------------------------------" << endl;
        cout << "Total Time = " << sc_time_stamp() << endl;
        cout << "Run Cycles: " << dec << run_cycles << endl;
        cout << "WFI Cycles: " << dec << wfi_cycles << endl;
        double energy = ((sc_uint<64>) run_cycles * 2e-09 * ACTIVE_POWER) + ((sc_uint<64>) wfi_cycles * 2e-09 * SLEEP_POWER);
        cout << "Estimated Core Power (W) = " << ((double) energy / sc_time_stamp().to_seconds()) << endl;
        cout << "GPIO1.0 ON cycles: " << dec << ((double) gpio10_count / 2) << endl;
        cout << "GPIO1.0 Duty cycle: " << dec << (1e-09 * (double) gpio10_count / sc_time_stamp().to_seconds()) << endl;
        cout << "GPIO1.1 ON cycles: " << dec << ((double) gpio11_count / 2) << endl;
        cout << "GPIO1.1 Duty cycle: " << dec << (1e-09 * (double) gpio11_count / sc_time_stamp().to_seconds()) << endl;

        cout << "---------------------------------------" << endl;
        cout << "Core Energy(J): " << dec << energy << endl;
        energy += (double) gpio10_count * 1E-09 * GPIO10_POWER;
        energy += (double) gpio11_count * 1E-09 * GPIO11_POWER;
        cout << "Temperature Sensor Energy(J): " << ((double) gpio10_count * 1E-09 * GPIO10_POWER) << endl;
        cout << "Display Energy(J): " << ((double) gpio11_count * 1E-09 * GPIO11_POWER) << endl;
        cout << "=======================================" << endl;
        cout << "Total Energy(J) = " << dec << energy << endl;
        cout << "=======================================" << endl;
        cout << "Estimated System Power(W): " << (energy / sc_time_stamp().to_seconds()) << endl;
        cout << "=======================================" << endl;
        // cout << sc_time_stamp() << endl;
        sc_stop();
    }

    void stop_on_halt() {
        if(halt.read()) {
            sc_stop();
        }
    }

    void systick_interrupt_emulate(void) {
        if (((sc_uint<64>) clock_count % SLEEP_PERIOD == 0) && (((sc_uint<64>) clock_count) > 0)) {
            emu_systick_int.write(true);
        } else if ((sc_uint<32>)interrupt_flag > 0) {
            emu_systick_int.write(false);
        }
    }

    void interrupt_generator(void) {
        // cout << "Interrupt Value: " << hex << (sc_uint<32>) internal_interrupts.read() << endl;
        // cout << sc_time_stamp() << " - SysTick Interrupt Value: " << hex << (sc_uint<32>) emu_systick_int.read() << endl;
        // cout << sc_time_stamp() << " - Full Interrupt Value: " << hex << (((sc_uint<32>) internal_interrupts.read()) | ((sc_uint<32>) emu_systick_int.read())) << endl;
        interrupt.write(((sc_uint<32>) internal_interrupts.read()) | ((sc_uint<32>) emu_systick_int.read()));
    }

    void monitor_proc() {
        cout << "Time: " << sc_time_stamp() << endl;
    }

    void gpio10_monitor() {
        if (gpio1_inout.read()[0] == SC_LOGIC_1) {
            gpio10_count = (sc_uint<64>) gpio10_count + 1;
        }
    }

    void gpio11_monitor() {
        if (gpio1_inout.read()[1] == SC_LOGIC_1) {
            gpio11_count = (sc_uint<64>) gpio11_count + 1;
        }
    }

    SC_CTOR(Testbench) {
        clock_count = 0;
        gpio10_count = 0;
        gpio11_count = 0;
        emu_systick_int = false;
        interrupt.write(0x000000000);
        rv32i = new RV32I("rv32i");
        rv32i->clk(clk);
        rv32i->nreset(nreset);
        rv32i->interrupt(interrupt);
        rv32i->interrupt_flag(interrupt_flag);
        rv32i->halt(halt);
        rv32i->run_cycles(run_cycles);
        rv32i->wfi_cycles(wfi_cycles);
        rv32i->gpio0_inout(gpio0_inout);
        rv32i->gpio1_inout(gpio1_inout);

        // sc_trace_file *tf;
        // tf = sc_create_vcd_trace_file("waveform");

        // sc_trace(tf, rv32i->pc, "pc");
        // sc_trace(tf, rv32i->registers, "registers");
        // sc_trace(tf, rv32i->instruction_memory, "instruction_memory");
        // sc_trace(tf, rv32i->data_memory, "data_memory");

        SC_THREAD(clk_gen);
        SC_THREAD(stim_proc);

        SC_METHOD(systick_interrupt_emulate);
        sensitive << clock_count << interrupt_flag;

        SC_METHOD(interrupt_generator);
        sensitive << internal_interrupts << emu_systick_int;

        SC_METHOD(stop_on_halt);
        sensitive << clk;

        SC_METHOD(gpio10_monitor);
        sensitive << clk;

        SC_METHOD(gpio11_monitor);
        sensitive << clk;
        // SC_METHOD(monitor_proc);
    }
};

int sc_main(int argc, char* argv[]) {
    Testbench tb("tb");
    sc_start();
    return 0;
}
