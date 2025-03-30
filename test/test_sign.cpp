#include <systemc.h>

SC_MODULE(Testbench) {
    sc_int<32> sign_extend_12(sc_uint<12> num) {
        if (num[11]) { // If the most significant bit is 1 (negative)
            return (sc_int<32>) -((1 << 12) - num);
        } else { // If the most significant bit is 0 (positive)
            return (sc_int<32>) num;
        }
    }
    
    void run_test() {
        sc_uint<12> values[] = { 0x7FF, 0x800, 0xFFF, 0x001 }; // Test cases
        sc_int<32> extended_value;

        cout << "12-bit Unsigned | Interpreted Signed | 32-bit Sign Extended" << endl;
        cout << "-------------------------------------------------------" << endl;

        for (int i = 0; i < 4; i++) {
            sc_uint<12> x = values[i];

            // Sign extension by casting to sc_int<12> first
            extended_value = sign_extend_12(x);

            // Print results
            cout << hex << "0x" << x.to_uint() << "          |  "
                 << (sc_int<12>)x << "              |  "
                 << extended_value << endl;
        }
    }

    SC_CTOR(Testbench) {
        SC_THREAD(run_test);
    }
};

int sc_main(int argc, char* argv[]) {
    Testbench tb("tb");
    sc_start();
    return 0;
}
