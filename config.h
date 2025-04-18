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