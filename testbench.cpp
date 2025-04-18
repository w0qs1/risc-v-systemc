#include <systemc.h>
#include "rv32i.cpp"
//#include <tracing.h>

SC_MODULE(Testbench) {
    sc_signal<bool> clk, nreset, halt;
    sc_signal_rv<32> gpio0_inout, gpio1_inout;
    sc_signal<sc_uint<32>> interrupt;
    RV32I *rv32i;

    void clk_gen() {
        while (true) {
            clk.write(false);
            wait(1, SC_NS);
            clk.write(true);
            wait(1, SC_NS);
        }
    }

    void stim_proc() {
        nreset.write(false);
        interrupt.write(0x000000000);
        gpio0_inout.write(sc_lv<32>("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"));
        gpio1_inout.write(sc_lv<32>("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"));
        wait(2, SC_NS);
        nreset.write(true);
        wait(10, SC_NS);
        interrupt.write(0x000000002);
        wait(10, SC_NS);
        interrupt.write(0x000000000);
        wait(30, SC_NS);
        gpio1_inout.write(sc_lv<32>("01010101010101010101010101010101"));
        wait(30, SC_NS);
        gpio1_inout.write(sc_lv<32>("10101010101010101010101010101010"));
        wait(10, SC_NS);
    }

    void stop_on_halt() {
        if(halt.read()) {
            sc_stop();
        }
    }

    void monitor_proc() {
        cout << "Time: " << sc_time_stamp() << endl;
    }

    SC_CTOR(Testbench) {
        rv32i = new RV32I("rv32i");
        rv32i->clk(clk);
        rv32i->nreset(nreset);
        rv32i->interrupt(interrupt);
        rv32i->halt(halt);
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
        SC_METHOD(stop_on_halt);
        // SC_METHOD(monitor_proc);
        sensitive << clk;
    }
};

int sc_main(int argc, char* argv[]) {
    Testbench tb("tb");
    sc_start();
    return 0;
}
