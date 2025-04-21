#include <systemc.h>

SC_MODULE(SysTick) {
    // Ports
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_in<bool> int_enable;
    sc_in<bool> s_write;
    sc_in<bool> s_read;
    sc_in<sc_uint<2>> r_sel;       // 2-bit address: 0 - CSR, 1 - TIMER, 2 - MATCH
    sc_in<sc_uint<32>> wdata;
    sc_out<sc_uint<32>> rdata;
    sc_out<bool> irq;

    // Internal registers
    sc_signal<sc_uint<32>> timer_val;
    sc_signal<sc_uint<32>> match_val;
    sc_signal<sc_uint<32>> csr;  // [0]=enable

    sc_uint<32> csr_temp;
    sc_uint<32> tval_temp;
    bool irq_flag;

    bool timer_val_written;

    void tick_process() {
        sc_uint<32> tval;
        if (nreset.read() == false) {
            timer_val.write(0);
            irq.write(false);
            irq_flag = false;
        } else {
            csr_temp = csr.read();
            
            if (csr_temp[0]) {  // Timer enabled
                if (timer_val_written == true) {
                    tval = tval_temp + 1;
                    timer_val.write(tval);
                    timer_val_written = false;
                } else {
                    tval = timer_val.read();
                    tval++;
                    timer_val.write(tval);
                }
                // cout << "SysTick Timer value: " << dec << timer_val.read() << endl;

                if (tval == match_val.read()) {
                    if (int_enable) {
                        // cout << "SysTick Match Interrupt at " << sc_time_stamp() << endl;
                        irq_flag = true;
                    }
                    timer_val.write(0);
                }
            } else if(~int_enable) {
                irq_flag = false;
            }
        }

        irq.write(irq_flag);
    }    

    void bus_interface() {
        if (nreset.read() == false) {
            rdata.write(0);
            csr.write(0);
        } else {
            if (s_write.read()) {
                switch (r_sel.read()) {
                    case 0: // CSR
                        csr_temp = wdata.read(); //.range(2,0);
                        // cout << "SysTick CSR Write: " << csr_temp << endl;
                        break;
                    case 1: // Timer value set
                        tval_temp = wdata.read();
                        // cout << "SysTick Timer write: " << tval_temp << endl;
                        timer_val_written = true;
                        break;
                    case 2: // MATCH
                        match_val.write(wdata.read());
                        tval_temp = 0;
                        timer_val_written = true;
                        break;
                    default: break; // TIMER is read-only
                }
            } else if (s_read.read()) { // Read
                switch (r_sel.read()) {
                    case 0:
                        rdata.write(csr.read());
                        // cout << "SysTick CSR read: " << csr.read() << endl;
                        break;
                    case 1: 
                        rdata.write(timer_val.read());
                        // cout << "SysTick Timer read: " << dec << timer_val.read() << " | at Time:  " << sc_time_stamp() << endl;
                        break;
                    case 2:
                        rdata.write(match_val.read());
                        // cout << "SysTick Match read: " << match_val.read() << endl;
                        break;
                    default:
                        rdata.write(0);
                        break;
                }
            }
        }
        csr.write(csr_temp);
    }

    SC_CTOR(SysTick) {
        SC_METHOD(tick_process);
        sensitive << clk.pos();
        dont_initialize();

        SC_METHOD(bus_interface);
        sensitive << clk.pos();
        dont_initialize();
    }
};