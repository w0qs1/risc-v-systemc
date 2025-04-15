#include <systemc.h>

SC_MODULE(GPIO) {
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_in<sc_uint<2>> r_sel;
    sc_in<bool> s_write;
    sc_inout<sc_uint<32>> data_inout;

    sc_out<sc_uint<32>> csr;
    sc_out<sc_uint<32>> ddr;
    sc_out<sc_uint<32>> odr;
    sc_out<sc_uint<32>> idr;

    sc_inout<sc_uint<32>> gpio_inout;

    void gpio_handle(void) {
        if(nreset == false) {
            csr.write(0x112233AA);
            ddr.write(0);
            odr.write(0);
            idr.write(0);
        } else if (s_write == true) {
            if (r_sel.read() == 0) {
                cout << "GPIO: CSR write" << endl;
                csr.write(data_inout);
            } else if (r_sel.read() == 1) {
                cout << "GPIO: DDR write" << endl;
                ddr.write(data_inout);
            } else if (r_sel.read() == 2) {
                cout << "GPIO: ODR write" << endl;
                odr.write(data_inout);
            }
        } else if (s_write == false) {
            if (r_sel.read() == 0) {
                // cout << "GPIO: CSR read" << endl;
                data_inout = csr.read();
            } else if (r_sel.read() == 1) {
                // cout << "GPIO: DDR read" << endl;
                data_inout = ddr.read();
            } else if (r_sel.read() == 2) {
                // cout << "GPIO: ODR read" << endl;
                data_inout = odr.read();
            } else if (r_sel.read() == 3) {
                // cout << "GPIO: IDR read" << endl;
                data_inout = idr.read();
            }
        }
    }
    SC_CTOR(GPIO) {
        SC_METHOD(gpio_handle);
        sensitive << nreset << r_sel << s_write;
    }
};