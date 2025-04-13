#include <systemc.h>

SC_MODULE(GPIO) {
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_in<sc_uint<2>> r_sel;
    sc_in<bool> s_write;
    sc_in<sc_uint<32>> data_in;
    sc_out<sc_uint<32>> csr;
    sc_out<sc_uint<32>> ddr;
    sc_out<sc_uint<32>> odr;
    sc_out<sc_uint<32>> idr;

    void gpio_handle(void) {
        if(nreset == false) {
            csr.write(0);
            ddr.write(0);
            odr.write(0);
            idr.write(0);
        } else if (s_write == true){
            if (r_sel.read() == 0) {
                csr.write(data_in);
            } else if (r_sel.read() == 1) {
                ddr.write(data_in);
            } else if (r_sel.read() == 2) {
                odr.write(data_in);                
            }
        }
    }
    SC_CTOR(GPIO) {
        SC_THREAD(gpio_handle);
        sensitive << clk.pos() << nreset << s_write;
    }
};