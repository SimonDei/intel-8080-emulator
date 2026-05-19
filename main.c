#include "cpu.h"
#include "bus.h"
#include "instructions.h"

#define R_B 0
#define R_C 1
#define R_D 2
#define R_E 3
#define R_H 4
#define R_L 5
#define R_M 6
#define R_A 7

#define EMIT(cpu, byte) ((&cpu)->bus->memory.data[(&cpu)->asm_pc++] = (uint8_t)(byte))
#define ORG(cpu, addr) ((&cpu)->asm_pc = (uint16_t)(addr))
#define LABEL(cpu, name) uint16_t name = (&cpu)->asm_pc

#define INR(cpu, r) (EMIT(cpu, 0x04 | ((r) << 3)))
#define DCR(cpu, r) (EMIT(cpu, 0x05 | ((r) << 3)))
#define MVI(cpu, r, imm) \
    do { EMIT(cpu, 0x06 | ((r) << 3)); EMIT(cpu, (imm)); } while (0)
#define RLC(cpu) (EMIT(cpu, 0x07))
#define RRC(cpu) (EMIT(cpu, 0x0F))
#define RAL(cpu) (EMIT(cpu, 0x17))
#define RAR(cpu) (EMIT(cpu, 0x1F))
#define SHLD(cpu, addr) \
    do { \
        EMIT(cpu, 0x22); \
        EMIT(cpu, (addr) & 0xFF); \
        EMIT(cpu, (addr) >> 8); \
    } while(0)
#define DAA(cpu) (EMIT(cpu, 0x27))
#define LHLD(cpu, addr) \
    do { \
        EMIT(cpu, 0x2A); \
        EMIT(cpu, (addr) & 0xFF); \
        EMIT(cpu, (addr) >> 8); \
    } while(0)
#define CMA(cpu) (EMIT(cpu, 0x2F))
#define STA(cpu, addr) \
    do { \
        EMIT(cpu, 0x32); \
        EMIT(cpu, (addr) & 0xFF); \
        EMIT(cpu, (addr) >> 8); \
    } while(0)
#define STC(cpu) (EMIT(cpu, 0x37))
#define LDA(cpu, addr) \
    do { \
        EMIT(cpu, 0x3A); \
        EMIT(cpu, (addr) & 0xFF); \
        EMIT(cpu, (addr) >> 8); \
    } while(0)
#define CMC(cpu) (EMIT(cpu, 0x3F))
#define MOV(cpu, dst, src) (EMIT(cpu, 0x40 | ((dst) << 3) | (src)))
#define HLT(cpu) (EMIT(cpu, 0x76))
#define ADD(cpu, r) (EMIT(cpu, 0x80 | (r)))
#define ADC(cpu, r) (EMIT(cpu, 0x88 | (r)))
#define SUB(cpu, r) (EMIT(cpu, 0x90 | (r)))
#define SBB(cpu, r) (EMIT(cpu, 0x98 | (r)))
#define ANA(cpu, r) (EMIT(cpu, 0xA0 | (r)))
#define XRA(cpu, r) (EMIT(cpu, 0xA8 | (r)))
#define ORA(cpu, r) (EMIT(cpu, 0xB0 | (r)))
#define CMP(cpu, r) (EMIT(cpu, 0xB8 | (r)))
#define POP_B(cpu) (EMIT(cpu, 0xC1))
#define JNZ(cpu, label) \
    do { \
        EMIT(cpu, 0xC2); \
        EMIT(cpu, (label) & 0xFF); \
        EMIT(cpu, (label) >> 9); \
    } while (0)
#define JMP(cpu, label) \
    do { \
        EMIT(cpu, 0xC3); \
        EMIT(cpu, (label) & 0xFF); \
        EMIT(cpu, (label) >> 8); \
    } while (0)
#define CNZ(cpu, addr) \
    do { \
        EMIT(cpu, 0xC4); \
        EMIT(cpu, (addr) & 0xFF); \
        EMIT(cpu, (addr) >> 8); \
    } while(0)
#define PUSH_B(cpu) (EMIT(cpu, 0xC5))
#define RET(cpu) (EMIT(cpu, 0xC9))
#define OUT(cpu, port) \
    do { EMIT(cpu, 0xD3); EMIT(cpu, (port)); } while (0)
#define IN(cpu, port) \
    do { EMIT(cpu, 0xDB); EMIT(cpu, (port)); } while (0)

int main(void) {
    CPU cpu = { 0 };
    BUS bus = { 0 };

    bus_allocate_memory(&bus, 64 * 1024); // Allocate 64 KB of memory for the bus

    cpu_connect_bus(&cpu, &bus);
    cpu_reset(&cpu);

    cpu.io_out[0] = 10;

    MVI(cpu, R_B, 0x12);
    MVI(cpu, R_C, 0x34);
    PUSH_B(cpu);

    // read from port 0 and store it in C
    IN(cpu, 0);
    MOV(cpu, R_C, R_A);

    // clear A, B and move 10 into D
    XRA(cpu, R_A);
    MVI(cpu, R_B, 0);
    MVI(cpu, R_D, 10);

    LABEL(cpu, loop);
    
    // move B into A and add C to it
    MOV(cpu, R_A, R_B);
    ADD(cpu, R_C);
    
    // store A's result into B
    MOV(cpu, R_B, R_A);
    
    // decrement D by 1 and store it in A
    DCR(cpu, R_D);
    MOV(cpu, R_A, R_D);
    
    // jump back if D's value is > 0
    JNZ(cpu, loop);

    // move the result from B back into A
    MOV(cpu, R_A, R_B);

    // multiply the result by 2 and output it to port 1
    RLC(cpu);
    OUT(cpu, 1);
    
    POP_B(cpu);

    // halt the program
    HLT(cpu);

    cpu_run(&cpu);
    cpu_log_registers(&cpu);
    cpu_log_ports(&cpu);
    bus_dump_memory_range(&bus, 0, 255);

    bus_free_memory(&bus);
    return 0;
}
