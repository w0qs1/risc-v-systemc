#ifndef CONFIG_H
#define CONFIG_H
// Size of instruction memory in number of words (4B)
#define INSTR_MEM_SIZE  2048

// Size of data memory in number of Bytes (includes peripheral memory as well)
#define DATA_MEM_SIZE   576
#define DATA_MEM_END    512

// GPIO0 and GPIO1
#define GPIO0_BASE      DATA_MEM_END
#define GPIO0_END       GPIO0_BASE + (4 * 4)
#define GPIO1_BASE      GPIO0_END
#define GPIO1_END       GPIO1_BASE + (4 * 4)

// SysTick Timer
#define SYSTICK_BASE    GPIO1_END
#define SYSTICK_END     SYSTICK_BASE + (3 * 4)

#define SLEEP_PERIOD    512

#define STOP_TIME       32768

// Power configuration
#define R_TYPE_POWER    2500E-12
#define I_TYPE_POWER    10000E-12
#define S_TYPE_POWER    10000E-12
#define U_TYPE_POWER    6500E-12
#define B_TYPE_POWER    8000E-12
#define J_TYPE_POWER    7500E-12
#define Y_TYPE_POWER    3500E-12
#define X_TYPE_POWER    3000E-12

// #define ACTIVE_POWER    1000E-06
#define SLEEP_POWER     10E-06

// Temperature Sensor Power
#define GPIO10_POWER    20E-03

// Dispaly Power
#define GPIO11_POWER    80E-03

// // Battery Capacity (Ah)
// #define BAT_CAPACITY    2500E-03
// // Battery Nominal Voltage (V)
// #define BAT_VOLTAGE     3.7

// Battery Capacity (Ah)
#define BAT_CAPACITY    225E-03
// Battery Nominal Voltage (V)
#define BAT_VOLTAGE     3

#define BAT_CAP_WH      BAT_CAPACITY * BAT_VOLTAGE

#endif