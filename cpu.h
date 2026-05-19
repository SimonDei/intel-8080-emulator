#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct BUS BUS;

typedef struct {
    uint8_t value;
} Register8;

typedef union {
    struct {
        uint8_t carry : 1;
        uint8_t pad1  : 1; /* always 1 */
        uint8_t parity : 1;
        uint8_t pad3  : 1; /* always 0 */
        uint8_t auxiliary_carry : 1;
        uint8_t pad5  : 1; /* always 0 */
        uint8_t zero  : 1;
        uint8_t sign  : 1;
    } f;
    uint8_t psw;
} FlagRegister;

typedef struct {
    uint16_t value;
} Register16;

typedef struct CPU {
    Register8 A;
    Register8 B;
    Register8 C;
    Register8 D;
    Register8 E;
    Register8 H;
    Register8 L;
    FlagRegister flags;
    Register16 SP;
    Register16 PC;
    BUS* bus;
    uint32_t cycles;
    uint8_t halted;
    uint16_t asm_pc;
    uint8_t io_out[256];
} CPU;

void cpu_connect_bus(CPU* cpu, BUS* bus);
void cpu_write_memory(CPU* cpu, uint16_t address, uint8_t value);
void cpu_write_memory_seq(CPU* cpu, uint8_t* values, uint16_t values_len);
uint8_t cpu_read_memory(const CPU* cpu, uint16_t address);
void cpu_write_port(CPU* cpu, uint8_t port, uint8_t value);
uint8_t cpu_read_port(CPU* cpu, uint8_t port);
void cpu_reset(CPU* cpu);
int cpu_execute_instruction(CPU* cpu);
void cpu_run_frame(CPU* cpu);
void cpu_run(CPU* cpu);
void cpu_log_registers(const CPU* cpu);
void cpu_log_ports(const CPU* cpu);

#endif
