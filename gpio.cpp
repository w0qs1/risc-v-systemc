// #include <systemc.h>

// SC_MODULE(GPIO) {
//     sc_in<bool> clk;
//     sc_in<bool> nreset;
//     sc_in<sc_uint<2>> r_sel;
//     sc_in<bool> s_write;
//     sc_in<bool> s_read;
//     sc_in<sc_uint<32>> data_in;
//     sc_out<sc_uint<32>> data_out;

//     sc_inout<sc_uint<32>> gpio_inout;

//     sc_signal<sc_uint<32>> ddr_reg, odr_reg, csr_reg, idr_reg;

//     void gpio_handle(void) {
//         if(nreset == false) {
//             csr_reg.write(0);
//             ddr_reg.write(0);
//             odr_reg.write(0);
//             idr_reg.write(0);
//         } else if (s_write == true && s_read == false) {
//             if (r_sel.read() == 0) {
//                 // cout << "GPIO: CSR write" << endl;
//                 csr_reg.write(data_in);
//             } else if (r_sel.read() == 1) {
//                 // cout << "GPIO: DDR write" << endl;
//                 ddr_reg.write(data_in);
//             } else if (r_sel.read() == 2) {
//                 // cout << "GPIO: ODR write" << endl;
//                 odr_reg.write(data_in);
//             }
//         } else if (s_read == true && s_write == false) {
//             if (r_sel.read() == 0) {
//                 // cout << "GPIO: CSR read" << endl;
//                 data_out.write(csr_reg.read());
//             } else if (r_sel.read() == 1) {
//                 // cout << "GPIO: DDR read" << endl;
//                 data_out.write(ddr_reg.read());
//             } else if (r_sel.read() == 2) {
//                 // cout << "GPIO: ODR read" << endl;
//                 data_out.write(odr_reg.read());
//             } else if (r_sel.read() == 3) {
//                 // cout << "GPIO: IDR read" << endl;
//                 data_out.write(idr_reg.read());
//             }
//         }
//     }

//     void gpio_logic() {
//         if (nreset == true) {
//             sc_uint<32> gpio_val = gpio_inout.read();
//             sc_uint<32> out_val = 0;
//             sc_uint<32> idr_val = idr_reg.read();
        
//             for (int i = 0; i < 32; i++) {
//                 if (ddr_reg.read()[i] == 1) {
//                     // Output mode: drive pin
//                     out_val[i] = odr_reg.read()[i];
//                 } else {
//                     // Input mode: read from pin
//                     idr_val[i] = gpio_val[i];
//                 }
//             }
        
//             gpio_inout.write(out_val);
//             idr_reg.write(idr_val);  // update the input register
//             cout << "Output Value: " << hex << out_val << endl;
//             // cout << "Input Value: " << hex << idr_val << endl;
//         }
        
//     }

//     SC_CTOR(GPIO) {
//         SC_METHOD(gpio_handle);
//         sensitive << clk.pos();

//         SC_METHOD(gpio_logic);
//         sensitive << ddr_reg << odr_reg << gpio_inout;
//     }
// };

#include <systemc.h>
#include <iomanip>

using namespace std;

SC_MODULE(GPIO) {
    sc_in<bool> clk;
    sc_in<bool> nreset;
    sc_in<sc_uint<2>> r_sel;
    sc_in<bool> s_write;
    sc_in<bool> s_read;
    sc_in<sc_uint<32>> data_in;
    sc_out<sc_uint<32>> data_out;

    sc_inout<sc_uint<32>> gpio_inout;

    sc_signal<sc_uint<32>> ddr_reg, odr_reg, csr_reg, idr_reg;
    sc_uint<32> out_val;

    void gpio_handle(void) {
        if (!nreset.read()) {
            csr_reg.write(0);
            ddr_reg.write(0);
            odr_reg.write(0);
        } else if (s_write.read() && !s_read.read()) {
            switch (r_sel.read()) {
                case 0:
                    csr_reg.write(data_in.read());
                    // cout << "CSR Write: 0x" << hex << data_in.read() << endl;
                    break;
                case 1:
                    ddr_reg.write(data_in.read());
                    // cout << "DDR Write: 0x" << hex << data_in.read() << endl;
                    break;
                case 2:
                    odr_reg.write(data_in.read());
                    // cout << "ODR Write: 0x" << hex << data_in.read() << endl;
                    break;
            }
        } else if (s_read.read() && !s_write.read()) {
            switch (r_sel.read()) {
                case 0:
                    data_out.write(csr_reg.read());
                    // cout << "CSR Read: 0x" << hex << csr_reg.read() << endl;
                    break;
                case 1:
                    data_out.write(ddr_reg.read());
                    // cout << "DDR Read: 0x" << hex << ddr_reg.read() << endl;
                    break;
                case 2:
                    data_out.write(odr_reg.read());
                    // cout << "ODR Read: 0x" << hex << odr_reg.read() << endl;
                    break;
                case 3:
                    data_out.write(idr_reg.read());
                    // cout << "IDR Read: 0x" << hex << idr_reg.read() << endl;
                    break;
            }
        }
    }

    void gpio_logic() {
        if (nreset.read() == false) {
            idr_reg.write(0);
        } else {
            sc_uint<32> gpio_val = gpio_inout.read();
            sc_uint<32> idr_val = idr_reg.read();
            out_val = 0;

            for (int i = 0; i < 32; i++) {
                if (ddr_reg.read()[i]) {
                    out_val[i] = odr_reg.read()[i]; // output mode
                } else {
                    idr_val[i] = gpio_val[i];       // input mode
                }
            }

            gpio_inout.write(out_val);
            idr_reg.write(idr_val);
        }
    }

    void gpio_monitor(void) {
        cout << sc_time_stamp() << " GPIO Output Value: 0x" << hex << setw(8) << setfill('0') << out_val.to_uint() << endl;
    }    

    SC_CTOR(GPIO) {
        SC_METHOD(gpio_handle);
        sensitive << clk.pos();

        SC_METHOD(gpio_logic);
        sensitive << ddr_reg << odr_reg << gpio_inout;

        SC_METHOD(gpio_monitor);
        sensitive << gpio_inout;
    }
};
