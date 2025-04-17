// Size of instruction memory in number of words (4B)
#define INSTR_MEM_SIZE  2048
// Size of data memory in number of Bytes (includes peripheral memory as well)
#define DATA_MEM_SIZE   576
// Offset of GPIO
#define GPIO0_BASE   512
#define GPIO0_END    GPIO0_BASE + (4 * 4)
#define GPIO1_BASE   GPIO0_END
#define GPIO1_END    GPIO1_BASE + (4 * 4)