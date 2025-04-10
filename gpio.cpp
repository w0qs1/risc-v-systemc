#include <systemc.h>

SC_MODULE(GPIO) {
    sc_inout<sc_uint<32>> csr;
    sc_inout<sc_uint<32>> ddr;
    sc_inout<sc_uint<32>> odr;
    sc_inout<sc_uint<32>> idr;

    SC_CTOR(GPIO) {

    }
};