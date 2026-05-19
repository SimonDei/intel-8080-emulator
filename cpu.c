#include "cpu.h"
#include "bus.h"
#include "timing.h"
#include "instructions.h"

#include <stdio.h>

void cpu_connect_bus(CPU* cpu, BUS* bus) {
    cpu->bus = bus;
}

void cpu_write_memory(CPU* cpu, uint16_t address, uint8_t value) {
    bus_write(cpu->bus, address, value);
}

void cpu_write_memory_seq(CPU* cpu, uint8_t* values, uint16_t values_len) {
    for (uint16_t i = 0; i < values_len; i++) {
        bus_write(cpu->bus, i, values[i]);
    }
}

uint8_t cpu_read_memory(const CPU* cpu, uint16_t address) {
    return bus_read(cpu->bus, address);
}

void cpu_write_port(CPU* cpu, uint8_t port, uint8_t value) {
    cpu->io_out[port] = value;
}

uint8_t cpu_read_port(CPU* cpu, uint8_t port) {
    return cpu->io_out[port];
}

void cpu_reset(CPU* cpu) {
    cpu->A.value = 0;
    cpu->B.value = 0;
    cpu->C.value = 0;
    cpu->D.value = 0;
    cpu->E.value = 0;
    cpu->H.value = 0;
    cpu->L.value = 0;
    cpu->flags.f.sign = 0;
    cpu->flags.f.zero = 0;
    cpu->flags.f.auxiliary_carry = 0;
    cpu->flags.f.parity = 0;
    cpu->flags.f.carry = 0;
    cpu->flags.f.pad1 = 1;
    cpu->flags.f.pad3 = 0;
    cpu->flags.f.pad5 = 0;
    cpu->SP.value = 0xFFFF; // Stack Pointer starts at the top of memory
    cpu->PC.value = 0x0000; // Program Counter starts at the beginning of memory
    cpu->cycles = 0;
    cpu->halted = 0;
}

int cpu_execute_instruction(CPU* cpu) {
    uint16_t curr_pc = cpu->PC.value;
    if (curr_pc >= cpu->bus->memory.capacity) {
        return -1;
    }

    const uint8_t opcode = cpu_read_memory(cpu, curr_pc);
    const Instruction* instruction = &instructions[opcode];

    const uint16_t next_pc = curr_pc + instruction->length;
    cpu->PC.value = next_pc; // Move the program counter to the next instruction

    if (instruction->handler) {
        instruction->handler(cpu, curr_pc, NULL);
    } else {
        return -1;
    }

    cpu->cycles += instruction->cycles;
    return 0;
}

void cpu_run_frame(CPU* cpu) {
    const uint32_t start = cpu->cycles;
    while ((cpu->cycles - start) < CYCLES_PER_FRAME) {
        if (cpu->halted || cpu_execute_instruction(cpu) != 0) {
            break;
        }
    }
}

void cpu_run(CPU* cpu) {
    while (!cpu->halted) {
        cpu_run_frame(cpu);
        cpu_throttle(cpu, TARGET_HZ);
    }
}

void cpu_log_registers(const CPU* cpu) {
    printf("A: 0x%02X (%d)\nB: 0x%02X (%d)\nC: 0x%02X (%d)\nD: 0x%02X (%d)\nE: 0x%02X (%d)\nH: 0x%02X (%d)\nL: 0x%02X (%d)\n",
        cpu->A.value, cpu->A.value,
        cpu->B.value, cpu->B.value,
        cpu->C.value, cpu->C.value,
        cpu->D.value, cpu->D.value,
        cpu->E.value, cpu->E.value,
        cpu->H.value, cpu->H.value,
        cpu->L.value, cpu->L.value);
    printf("Flags: S=%d Z=%d A=%d P=%d C=%d\n",
        cpu->flags.f.sign,
        cpu->flags.f.zero,
        cpu->flags.f.auxiliary_carry,
        cpu->flags.f.parity,
        cpu->flags.f.carry);
    printf("SP: 0x%04X (%d)\nPC: 0x%04X (%d)\nCycles: %u\n",
        cpu->SP.value, cpu->SP.value,
        cpu->PC.value, cpu->PC.value,
        cpu->cycles);
}

void cpu_log_ports(const CPU* cpu) {
    printf("I/O Port State:\n");

    for (uint16_t port = 0; port < 256; port++) {
        uint8_t value = cpu->io_out[port];

        // Only print ports that have been written to
        if (value != 0) {
            printf("  Port %02X: %02X (%d) \n", port, value, value);
        }
    }
}
