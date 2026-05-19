#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <corecrt.h>
#include <Windows.h>

#ifdef _MSC_VER
    #include <intrin.h>
    #define POPCNT(x) __popcnt((unsigned int)(x))
#else
    #define POPCNT(x) __builtin_popcount((unsigned int)(x))
#endif

#define UNUSED_PARAM(x) (void)(x)

#define HIGH_BYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#define LOW_BYTE(x) ((uint8_t)((x) & 0xFF))
#define MAKE_WORD(high, low) (((uint16_t)(high) << 8) | (uint16_t)(low))

#define TARGET_HZ 2000000
#define FPS 60
#define CYCLES_PER_FRAME (TARGET_HZ / FPS)   /* ~33,333 cycles */

typedef struct {
    size_t size;
    size_t capacity;
    uint8_t* data;
} Memory;

typedef struct {
    Memory memory;
} Bus;

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

typedef struct {
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
    Bus* bus;
    uint32_t cycles;
    uint8_t halted;
} CPU;

LARGE_INTEGER g_perf_freq;
LARGE_INTEGER g_start_time;
uint8_t g_timer_ok = 0;

void init_timer(void) {
    QueryPerformanceFrequency(&g_perf_freq);
    QueryPerformanceCounter(&g_start_time);
    g_timer_ok = 1;
}

void cpu_throttle(const CPU* cpu, uint32_t target_hz) {
    if (!g_timer_ok) init_timer();

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    /* How many microseconds should have passed for the cycles we executed? */
    const uint64_t expected_us = ((uint64_t)cpu->cycles * 1000000ULL) / target_hz;

    /* How many microseconds actually passed? */
    uint64_t elapsed_us = ((now.QuadPart - g_start_time.QuadPart) * 1000000ULL) / g_perf_freq.QuadPart;

    if (expected_us > elapsed_us) {
        DWORD sleep_ms = (DWORD)((expected_us - elapsed_us) / 1000);
        if (sleep_ms > 0) Sleep(sleep_ms);

        /* Busy-wait the remaining sub-millisecond time for accuracy */
        do {
            QueryPerformanceCounter(&now);
            elapsed_us = ((now.QuadPart - g_start_time.QuadPart) * 1000000ULL) / g_perf_freq.QuadPart;
        } while (elapsed_us < expected_us);
    }
}

typedef void(*OpcodeHandler)(CPU* cpu, uint16_t curr_pc, void* value);

typedef struct {
    uint8_t opcode;
    uint8_t length;
    uint8_t cycles;
    OpcodeHandler handler;
} Instruction;

void nop_handler(CPU* cpu, uint16_t curr_pc, void* value);

void lxi_b_d16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void stax_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inx_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_b_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void rlc_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dad_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void ldax_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcx_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_c_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_c_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_c_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void rrc_handler(CPU* cpu, uint16_t curr_pc, void* value);

void lxi_d_d16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void stax_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inx_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_d_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void ral_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dad_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void ldax_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcx_d_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_e_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_e_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_e_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void rar_handler(CPU* cpu, uint16_t curr_pc, void* value);

void lxi_h_d16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void shld_a16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inx_h_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_h_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_h_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_h_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void daa_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dad_h_handler(CPU* cpu, uint16_t curr_pc, void* value);
void lhld_a16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcx_h_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_l_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_l_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_l_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void cma_handler(CPU* cpu, uint16_t curr_pc, void* value);

void lxi_sp_d16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void sta_a16_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inx_sp_handler(CPU* cpu, uint16_t curr_pc, void* value);
void inr_m_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dcr_m_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mvi_m_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
void stc_handler(CPU* cpu, uint16_t curr_pc, void* value);
void dad_sp_handler(CPU* cpu, uint16_t curr_pc, void* value);
void lda_a16_handler(CPU* cpu, uint16_t curr_pc, void* value);

void hlt_handler(CPU* cpu, uint16_t curr_pc, void* value);
void mov_a_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void add_b_handler(CPU* cpu, uint16_t curr_pc, void* value);
void rst_0_handler(CPU* cpu, uint16_t curr_pc, void* value);
void ret_handler(CPU* cpu, uint16_t curr_pc, void* value);
void call_addr_handler(CPU* cpu, uint16_t curr_pc, void* value);

const Instruction instructions[256] = {
    [0x00] = { 0x00, 1,  4, nop_handler }, /* NOP */
    [0x01] = { 0x01, 3, 10, lxi_b_d16_handler }, /* LXI B,d16 */
    [0x02] = { 0x02, 1,  7, stax_b_handler }, /* STAX B */
    [0x03] = { 0x03, 1,  5, inx_b_handler }, /* INX B */
    [0x04] = { 0x04, 1,  5, inr_b_handler }, /* INR B */
    [0x05] = { 0x05, 1,  5, dcr_b_handler }, /* DCR B */
    [0x06] = { 0x06, 2,  7, mvi_b_d8_handler }, /* MVI B,d8 */
    [0x07] = { 0x07, 1,  4, rlc_handler }, /* RLC */
    [0x08] = { 0x08, 1,  4, nop_handler }, /* NOP */
    [0x09] = { 0x09, 1, 10, dad_b_handler }, /* DAD B */
    [0x0A] = { 0x0A, 1,  7, ldax_b_handler }, /* LDAX B */
    [0x0B] = { 0x0B, 1,  5, dcx_b_handler }, /* DCX B */
    [0x0C] = { 0x0C, 1,  5, inr_c_handler }, /* INR C */
    [0x0D] = { 0x0D, 1,  5, dcr_c_handler }, /* DCR C */
    [0x0E] = { 0x0E, 2,  7, mvi_c_d8_handler }, /* MVI C,d8 */
    [0x0F] = { 0x0F, 1,  4, rrc_handler }, /* RRC */

    [0x10] = { 0x10, 1,  4, nop_handler }, /* NOP */
    [0x11] = { 0x11, 3, 10, lxi_d_d16_handler }, /* LXI D,d16 */
    [0x12] = { 0x12, 1,  7, stax_d_handler }, /* STAX D */
    [0x13] = { 0x13, 1,  5, inx_d_handler }, /* INX D */
    [0x14] = { 0x14, 1,  5, inr_d_handler }, /* INR D */
    [0x15] = { 0x15, 1,  5, dcr_d_handler }, /* DCR D */
    [0x16] = { 0x16, 2,  7, mvi_d_d8_handler }, /* MVI D,d8 */
    [0x17] = { 0x17, 1,  4, ral_handler }, /* RAL */
    [0x18] = { 0x18, 1,  4, nop_handler }, /* NOP */
    [0x19] = { 0x19, 1, 10, dad_d_handler }, /* DAD D */
    [0x1A] = { 0x1A, 1,  7, ldax_d_handler }, /* LDAX D */
    [0x1B] = { 0x1B, 1,  5, dcx_d_handler }, /* DCX D */
    [0x1C] = { 0x1C, 1,  5, inr_e_handler }, /* INR E */
    [0x1D] = { 0x1D, 1,  5, dcr_e_handler }, /* DCR E */
    [0x1E] = { 0x1E, 2,  7, mvi_e_d8_handler }, /* MVI E,d8 */
    [0x1F] = { 0x1F, 1,  4, rar_handler }, /* RAR */

    [0x20] = { 0x20, 1,  4, nop_handler }, /* NOP */
    [0x21] = { 0x21, 3, 10, lxi_h_d16_handler }, /* LXI H,d16 */
    [0x22] = { 0x22, 3, 16, shld_a16_handler }, /* SHLD a16 */
    [0x23] = { 0x23, 1,  5, inx_h_handler }, /* INX H */
    [0x24] = { 0x24, 1,  5, inr_h_handler }, /* INR H */
    [0x25] = { 0x25, 1,  5, dcr_h_handler }, /* DCR H */
    [0x26] = { 0x26, 2,  7, mvi_h_d8_handler }, /* MVI H,d8 */
    [0x27] = { 0x27, 1,  4, daa_handler }, /* DAA */
    [0x28] = { 0x28, 1,  4, nop_handler }, /* NOP */
    [0x29] = { 0x29, 1, 10, dad_h_handler }, /* DAD H */
    [0x2A] = { 0x2A, 3, 16, lhld_a16_handler }, /* LHLD a16 */
    [0x2B] = { 0x2B, 1,  5, dcx_h_handler }, /* DCX H */
    [0x2C] = { 0x2C, 1,  5, inr_l_handler }, /* INR L */
    [0x2D] = { 0x2D, 1,  5, dcr_l_handler }, /* DCR L */
    [0x2E] = { 0x2E, 2,  7, mvi_l_d8_handler }, /* MVI L,d8 */
    [0x2F] = { 0x2F, 1,  4, cma_handler }, /* CMA */

    [0x30] = { 0x30, 1,  4, nop_handler }, /* NOP */
    [0x31] = { 0x31, 3, 10, lxi_sp_d16_handler }, /* LXI SP,d16 */
    [0x32] = { 0x32, 3, 13, sta_a16_handler }, /* STA a16 */
    [0x33] = { 0x33, 1,  5, inx_sp_handler }, /* INX SP */
    [0x34] = { 0x34, 1, 10, inr_m_handler }, /* INR M */
    [0x35] = { 0x35, 1, 10, dcr_m_handler }, /* DCR M */
    [0x36] = { 0x36, 2, 10, mvi_m_d8_handler }, /* MVI M,d8 */
    [0x37] = { 0x37, 1,  4, stc_handler }, /* STC */
    [0x38] = { 0x38, 1,  4, nop_handler }, /* NOP */
    [0x39] = { 0x39, 1, 10, dad_sp_handler }, /* DAD SP */
    [0x3A] = { 0x3A, 3, 13, lda_a16_handler }, /* LDA a16 */

    [0x76] = { 0x76, 1,  7, hlt_handler }, /* HLT */
    [0x78] = { 0x78, 1,  5, mov_a_b_handler }, /* MOV A,B */
    [0x80] = { 0x80, 1,  4, add_b_handler }, /* ADD B */
    [0xC7] = { 0xC7, 1, 11, rst_0_handler }, /* RST 0 */
    [0xC9] = { 0xC9, 1, 10, ret_handler }, /* RET */
    [0xCD] = { 0xCD, 3, 17, call_addr_handler }, /* CALL addr */
};

void bus_allocate_memory(Bus* bus, size_t capacity) {
    bus->memory.size = 0;
    bus->memory.capacity = capacity;
    bus->memory.data = calloc(capacity, sizeof(uint8_t));
}

void bus_free_memory(Bus* bus) {
    free(bus->memory.data);
    bus->memory.data = NULL;
    bus->memory.size = 0;
    bus->memory.capacity = 0;
}

void bus_dump_memory_range(const Bus* bus, uint16_t start, uint16_t end) {
    if (start >= bus->memory.capacity || end >= bus->memory.capacity || start > end) {
        printf("Invalid memory range for dump.\n");
        return;
    }
    
    for (uint16_t addr = start; addr <= end; addr++) {
        if ((addr - start) % 16 == 0) {
            printf("\n0x%04X: ", addr);
        }
        printf("%02X ", bus->memory.data[addr]);
    }
    printf("\n");
}

void bus_dump_memory(const Bus* bus) {
    bus_dump_memory_range(bus, 0, (uint16_t)bus->memory.capacity - 1);
}

uint8_t bus_read(const Bus* bus, uint16_t address) {
    if (address < bus->memory.capacity) {
        return bus->memory.data[address];
    }
    return 0;
}

void bus_write(Bus* bus, uint16_t address, uint8_t value) {
    if (address < bus->memory.capacity) {
        bus->memory.data[address] = value;
    }
}

void cpu_connect_bus(CPU* cpu, Bus* bus) {
    cpu->bus = bus;
}

void cpu_write_memory(CPU* cpu, uint16_t address, uint8_t value) {
    bus_write(cpu->bus, address, value);
}

uint8_t cpu_read_memory(const CPU* cpu, uint16_t address) {
    return bus_read(cpu->bus, address);
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

errno_t cpu_execute_instruction(CPU* cpu) {
    uint16_t curr_pc = cpu->PC.value;
    if (curr_pc >= cpu->bus->memory.capacity) {
        return EINVAL;
    }

    const uint8_t opcode = cpu_read_memory(cpu, curr_pc);
    const Instruction* instruction = &instructions[opcode];

    const uint16_t next_pc = curr_pc + instruction->length;
    cpu->PC.value = next_pc; // Move the program counter to the next instruction

    if (instruction->handler) {
        instruction->handler(cpu, curr_pc, NULL);
    } else {
        return EINVAL;
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
    // make the output more readable by also showing decimal and better formatting
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

void nop_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(cpu);
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    /* NOP does nothing */
}

void lxi_b_d16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->B.value = HIGH_BYTE(d16); // Move the higher byte into register B
    cpu->C.value = LOW_BYTE(d16);  // Move the lower byte into register C
}

void stax_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the address from BC
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the address in BC
}

void inx_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC
    bc = (bc + 1) & 0xFFFF; // Increment BC and wrap around at 16 bits
    cpu->B.value = HIGH_BYTE(bc); // Update register B with the new higher byte
    cpu->C.value = LOW_BYTE(bc);  // Update register C with the new lower byte
}

void inr_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->B.value + 1) & 0xFF; // Increment B and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->B.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->B.value = result; // Store the result back in B
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->B.value - 1) & 0xFF; // Decrement B and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->B.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->B.value = result; // Store the result back in B
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_b_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->B.value = d8; // Move the value into register B
}

void rlc_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t msb = (cpu->A.value & 0x80) >> 7; // Get the most significant bit
    
    cpu->A.value = ((cpu->A.value << 1) | msb) & 0xFF; // Rotate left and wrap around
    cpu->flags.f.carry = msb; // Set the carry flag to the original MSB
}

void dad_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC

    const uint32_t result = hl + bc; // Add BC to HL

    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void ldax_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the address from BC
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the address in BC into A
}

void dcx_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint16_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC
    bc = (bc - 1) & 0xFFFF; // Decrement BC and wrap around at 16 bits
    
    cpu->B.value = HIGH_BYTE(bc); // Update register B with the new higher byte
    cpu->C.value = LOW_BYTE(bc);  // Update register C with the new lower byte
}

void inr_c_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->C.value + 1) & 0xFF; // Increment C and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->C.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->C.value = result; // Store the result back in C
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_c_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->C.value - 1) & 0xFF; // Decrement C and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->C.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->C.value = result; // Store the result back in C
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_c_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->C.value = d8; // Move the value into register C
}

void rrc_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t lsb = cpu->A.value & 0x01; // Get the least significant bit

    cpu->A.value = ((cpu->A.value >> 1) | (lsb << 7)) & 0xFF; // Rotate right and wrap around
    cpu->flags.f.carry = lsb; // Set the carry flag to the original LSB
}

void lxi_d_d16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->D.value = HIGH_BYTE(d16); // Move the higher byte into register D
    cpu->E.value = LOW_BYTE(d16);  // Move the lower byte into register E
}

void stax_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the address from DE
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the address in DE
}

void inx_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    de = (de + 1) & 0xFFFF; // Increment DE and wrap around at 16 bits

    cpu->D.value = HIGH_BYTE(de); // Update register D with the new higher byte
    cpu->E.value = LOW_BYTE(de);  // Update register E with the new lower byte
}

void inr_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->D.value + 1) & 0xFF; // Increment D and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->D.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->D.value = result; // Store the result back in D
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->D.value - 1) & 0xFF; // Decrement D and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->D.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->D.value = result; // Store the result back in D
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_d_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->D.value = d8; // Move the value into register D
}

void ral_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t msb = (cpu->A.value & 0x80) >> 7; // Get the most significant bit
    const uint8_t carry_in = cpu->flags.f.carry; // Get the current carry flag

    cpu->A.value = ((cpu->A.value << 1) | carry_in) & 0xFF; // Rotate left and wrap around
    cpu->flags.f.carry = msb; // Set the carry flag to the original MSB
}

void dad_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    const uint32_t result = hl + de; // Add DE to HL

    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void ldax_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the address from DE
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the address in DE into A
}

void dcx_d_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint16_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    de = (de - 1) & 0xFFFF; // Decrement DE and wrap around at 16 bits
    
    cpu->D.value = HIGH_BYTE(de); // Update register D with the new higher byte
    cpu->E.value = LOW_BYTE(de);  // Update register E with the new lower byte
}

void inr_e_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->E.value + 1) & 0xFF; // Increment E and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->E.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->E.value = result; // Store the result back in E
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_e_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->E.value - 1) & 0xFF; // Decrement E and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->E.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->E.value = result; // Store the result back in E
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_e_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->E.value = d8; // Move the value into register E
}

void rar_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t lsb = cpu->A.value & 0x01; // Get the least significant bit
    const uint8_t carry_in = cpu->flags.f.carry; // Get the current carry flag
    
    cpu->A.value = ((cpu->A.value >> 1) | (carry_in << 7)) & 0xFF; // Rotate right and wrap around
    cpu->flags.f.carry = lsb; // Set the carry flag to the original LSB
}

void lxi_h_d16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->H.value = HIGH_BYTE(d16); // Move the higher byte into register H
    cpu->L.value = LOW_BYTE(d16);  // Move the lower byte into register L
}

void shld_a16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction
    
    cpu_write_memory(cpu, addr, cpu->L.value); // Store the value of L at the lower byte of the address
    cpu_write_memory(cpu, addr + 1, cpu->H.value); // Store the value of H at the higher byte of the address
}

void inx_h_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    hl = (hl + 1) & 0xFFFF; // Increment HL and wrap around at 16 bits
    
    cpu->H.value = HIGH_BYTE(hl); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl);  // Update register L with the new lower byte
}

void inr_h_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->H.value + 1) & 0xFF; // Increment H and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((cpu->H.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->H.value = result; // Store the result back in H
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_h_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t result = (cpu->H.value - 1) & 0xFF; // Decrement H and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->H.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->H.value = result; // Store the result back in H
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_h_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->H.value = d8; // Move the value into register H
}

void daa_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t a = cpu->A.value;
    uint8_t correction = 0;

    if ((a & 0x0F) > 9 || cpu->flags.f.auxiliary_carry) {
        correction |= 0x06; /* Adjust lower nibble */
    }

    if (a > 0x99 || cpu->flags.f.carry) {
        correction |= 0x60; /* Adjust upper nibble */
    }

    const uint16_t result = a + correction;

    cpu->A.value = (uint8_t)(result & 0xFF);
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0;
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0;
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1);
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0;
    cpu->flags.f.auxiliary_carry = ((a & 0x0F) + (correction & 0x0F)) > 0x0F ? 1 : 0;
}

void dad_h_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t hl_result = hl + hl; // Add HL to itself (HL * 2)
    
    cpu->H.value = HIGH_BYTE(hl_result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl_result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (hl_result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void lhld_a16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction

    cpu->L.value = cpu_read_memory(cpu, addr); // Load the value at the lower byte of the address into L
    cpu->H.value = cpu_read_memory(cpu, addr + 1); // Load the value at the higher byte of the address into H
}

void dcx_h_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    hl = (hl - 1) & 0xFFFF; // Decrement HL and wrap around at 16 bits

    cpu->H.value = HIGH_BYTE(hl); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl);  // Update register L with the new lower byte
}

void inr_l_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->L.value + 1) & 0xFF; // Increment L and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->L.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->L.value = result; // Store the result back in L
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR does not affect the carry flag */
}

void dcr_l_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t result = (cpu->L.value - 1) & 0xFF; // Decrement L and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->L.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->L.value = result; // Store the result back in L
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR does not affect the carry flag */
}

void mvi_l_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->L.value = d8; // Move the value into register L
}

void cma_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = ~cpu->A.value; // Complement the value in register A
}

void lxi_sp_d16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value
    
    cpu->SP.value = d16; // Load the 16-bit value into the stack pointer
}

void sta_a16_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction
    
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the specified address
}

void inx_sp_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    cpu->SP.value = (cpu->SP.value + 1) & 0xFFFF; // Increment the stack pointer and wrap around at 16 bits
}

void inr_m_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t mem_value = cpu_read_memory(cpu, addr); // Read the value at the address
    const uint8_t result = (mem_value + 1) & 0xFF; // Increment the memory value and wrap around at 8 bits

    cpu->flags.f.auxiliary_carry = ((mem_value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu_write_memory(cpu, addr, result); // Store the result back in memory
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR M does not affect the carry flag */
}

void dcr_m_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t mem_value = cpu_read_memory(cpu, addr); // Read the value at the address
    const uint8_t result = (mem_value - 1) & 0xFF; // Decrement the memory value and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((mem_value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu_write_memory(cpu, addr, result); // Store the result back in memory
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR M does not affect the carry flag */
}

void mvi_m_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    
    cpu_write_memory(cpu, addr, d8); // Move the value into memory at the address in HL
}

void stc_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->flags.f.carry = 1; // Set the carry flag
}

void dad_sp_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t sp = cpu->SP.value; // Get the current value of SP
    const uint32_t result = hl + sp; // Add SP to HL
    
    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void lda_a16_handler(CPU* cpu, uint16_t curr_pc, void* value) {

}

void hlt_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->halted = 1; // Mark the CPU as halted
}

void mov_a_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->B.value; // Move the value from register B to A
}

/*
 * Flags affected: S, Z, A, P, C (Sign, Zero, Auxiliary Carry, Parity, Carry).
 */
void add_b_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->B.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->B.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void rst_0_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint16_t return_address = curr_pc + 1; /* address after RST instruction */
    const uint8_t hi = HIGH_BYTE(return_address);
    const uint8_t lo = LOW_BYTE(return_address);

    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, hi);
    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, lo);

    cpu->PC.value = 0x0000; /* Jump to the restart address (0x0000 for RST 0) */
}

void ret_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    cpu->SP.value += 1;

    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value);
    cpu->SP.value += 1;

    cpu->PC.value = MAKE_WORD(hi, lo); /* Jump back to the return address */
}

void call_addr_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1));
    const uint16_t ret = curr_pc + 3; /* address after CALL instruction */

    const uint8_t hi = HIGH_BYTE(ret);
    const uint8_t lo = LOW_BYTE(ret);

    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, hi);
    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, lo);

    cpu->PC.value = addr;
}

/** ***********************************************************************************************
 * Writes the NOP opcode to the memory offset location.
 *************************************************************************************************/
#define NOP(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x00); /* NOP opcode */ \
} while (0)

/** ***********************************************************************************************
 * BC <- #d16
 * Loads a 16-bit immediate value into the BC register pair.
 *************************************************************************************************/
#define LXI_B(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x01); /* LXI B,d16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(value)); /* Lower byte of d16 value */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(value)); /* Higher byte of d16 value */ \
} while (0)

/** ***********************************************************************************************
 * [BC] <- A
 * Stores the value of register A into the memory address pointed to by the BC register pair.
 *************************************************************************************************/
#define STAX_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x02); /* STAX B opcode */ \
} while (0)

/** ***********************************************************************************************
 * BC <- BC + 1
 * Increments the BC register pair by 1.
 *************************************************************************************************/
#define INX_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x03); /* INX B opcode */ \
} while (0)

/** ***********************************************************************************************
 * B <- B + 1
 * Increments the B register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x04); /* INR B opcode */ \
} while (0)

/** ***********************************************************************************************
 * B <- B - 1
 * Decrements the B register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x05); /* DCR B opcode */ \
} while (0)

/** ***********************************************************************************************
 * B <- #d8
 * Loads an 8-bit immediate value into the B register.
 *************************************************************************************************/
#define MVI_B(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x06); /* MVI B,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * A = A << 1
 * Rotates the bits in the A register to the left.
 * The most significant bit is moved to the least and also stored in the carry flag.
 *************************************************************************************************/
#define RLC(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x07); /* RLC opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL + BC
 * Adds the BC register pair to the HL register pair and updates the carry flag.
 *************************************************************************************************/
#define DAD_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x09); /* DAD B opcode */ \
} while (0)

/** ***********************************************************************************************
 * A <- [BC]
 * Loads the value from the memory address pointed to by the BC register pair into register A.
 *************************************************************************************************/
#define LDAX_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x0A); /* LDAX B opcode */ \
} while (0)

/** ***********************************************************************************************
 * BC <- BC - 1
 * Decrements the BC register pair by 1.
 *************************************************************************************************/
#define DCX_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x0B); /* DCX B opcode */ \
} while (0)

/** ***********************************************************************************************
 * C <- C + 1
 * Increments the C register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_C(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x0C); /* INR C opcode */ \
} while (0)

/** ***********************************************************************************************
 * C <- C - 1
 * Decrements the C register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_C(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x0D); /* DCR C opcode */ \
} while (0)

/** ***********************************************************************************************
 * C <- #d8
 * Loads an 8-bit immediate value into the C register.
 *************************************************************************************************/
#define MVI_C(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x0E); /* MVI C,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * A = A >> 1
 * Rotates the bits in the A register to the right.
 * The least significant bit is moved to the most and also stored in the carry flag.
 *************************************************************************************************/
#define RRC(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x0F); /* RRC opcode */ \
} while (0)

/** ***********************************************************************************************
 * DE <- #d16
 * Loads a 16-bit immediate value into the DE register pair.
 *************************************************************************************************/
#define LXI_D(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x11); /* LXI D,d16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(value)); /* Lower byte of d16 value */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(value)); /* Higher byte of d16 value */ \
} while (0)

/** ***********************************************************************************************
 * [DE] <- A
 * Stores the value of register A into the memory address pointed to by the DE register pair.
 *************************************************************************************************/
#define STAX_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x12); /* STAX D opcode */ \
} while (0)

/** ***********************************************************************************************
 * DE <- DE + 1
 * Increments the DE register pair by 1.
 *************************************************************************************************/
#define INX_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x13); /* INX D opcode */ \
} while (0)

/** ***********************************************************************************************
 * D <- D + 1
 * Increments the D register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x14); /* INR D opcode */ \
} while (0)

/** ***********************************************************************************************
 * D <- D - 1
 * Decrements the D register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x15); /* DCR D opcode */ \
} while (0)

/** ***********************************************************************************************
 * D <- #d8
 * Loads an 8-bit immediate value into the D register.
 *************************************************************************************************/
#define MVI_D(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x16); /* MVI D,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * A = (A << 1) | Carry
 * Rotates the bits in the A register to the left through the carry flag.
 * The most significant bit is moved to the least and also stored in the carry flag.
 *************************************************************************************************/
#define RAL(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x17); /* RAL opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL + DE
 * Adds the DE register pair to the HL register pair and updates the carry flag.
 *************************************************************************************************/
#define DAD_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x19); /* DAD D opcode */ \
} while (0)

/** ***********************************************************************************************
 * A <- [DE]
 * Loads the value from the memory address pointed to by the DE register pair into register A.
 *************************************************************************************************/
#define LDAX_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x1A); /* LDAX D opcode */ \
} while (0)

/** ***********************************************************************************************
 * DE <- DE - 1
 * Decrements the DE register pair by 1.
 *************************************************************************************************/
#define DCX_D(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x1B); /* DCX D opcode */ \
} while (0)

/** ***********************************************************************************************
 * E <- E + 1
 * Increments the E register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_E(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x1C); /* INR E opcode */ \
} while (0)

/** ***********************************************************************************************
 * E <- E - 1
 * Decrements the E register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_E(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x1D); /* DCR E opcode */ \
} while (0)

/** ***********************************************************************************************
 * E <- #d8
 * Loads an 8-bit immediate value into the E register.
 *************************************************************************************************/
#define MVI_E(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x1E); /* MVI E,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * A = (A >> 1) | (Carry << 7)
 * Rotates the bits in the A register to the right through the carry flag.
 * The least significant bit is moved to the most and also stored in the carry flag.
 *************************************************************************************************/
#define RAR(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x1F); /* RAR opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- #d16
 * Loads a 16-bit immediate value into the HL register pair.
 *************************************************************************************************/
#define LXI_H(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x21); /* LXI H,d16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(value)); /* Lower byte of d16 value */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(value)); /* Higher byte of d16 value */ \
} while (0)

/** ***********************************************************************************************
 * [a16] <- L, [a16+1] <- H
 * Stores the value of register L at the memory address specified by a16 and the value of register H at the next address.
 *************************************************************************************************/
#define SHLD(cpu, offset, addr) do { \
    cpu_write_memory(cpu, offset++, 0x22); /* SHLD a16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(addr)); /* Lower byte of a16 address */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(addr)); /* Higher byte of a16 address */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL + 1
 * Increments the HL register pair by 1.
 *************************************************************************************************/
#define INX_H(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x23); /* INX H opcode */ \
} while (0)

/** ***********************************************************************************************
 * H <- H + 1
 * Increments the H register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_H(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x24); /* INR H opcode */ \
} while (0)

/** ***********************************************************************************************
 * H <- H - 1
 * Decrements the H register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_H(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x25); /* DCR H opcode */ \
} while (0)

/** ***********************************************************************************************
 * H <- #d8
 * Loads an 8-bit immediate value into the H register.
 *************************************************************************************************/
#define MVI_H(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x26); /* MVI H,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * Adjusts the value in register A to a valid BCD representation after an addition operation.
 * This instruction is typically used after adding two BCD numbers to correct the result.
 *************************************************************************************************/
#define DAA(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x27); /* DAA opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL + HL
 * Adds the HL register pair to itself and updates the carry flag.
 *************************************************************************************************/
#define DAD_H(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x29); /* DAD H opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- [a16]
 * Loads the value from the memory address specified by a16 into the HL register pair.
 *************************************************************************************************/
#define LHLD(cpu, offset, addr) do { \
    cpu_write_memory(cpu, offset++, 0x2A); /* LHLD a16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(addr)); /* Lower byte of a16 address */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(addr)); /* Higher byte of a16 address */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL - 1
 * Decrements the HL register pair by 1.
 *************************************************************************************************/
#define DCX_H(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x2B); /* DCX H opcode */ \
} while (0)

/** ***********************************************************************************************
 * L <- L + 1
 * Increments the L register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define INR_L(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x2C); /* INR L opcode */ \
} while (0)

/** ***********************************************************************************************
 * L <- L - 1
 * Decrements the L register by 1 and updates the flags accordingly.
 *************************************************************************************************/
#define DCR_L(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x2D); /* DCR L opcode */ \
} while (0)

/** ***********************************************************************************************
 * L <- #d8
 * Loads an 8-bit immediate value into the L register.
 *************************************************************************************************/
#define MVI_L(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x2E); /* MVI L,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * A <- ~A
 * Complements the value in register A (bitwise NOT).
 *************************************************************************************************/
#define CMA(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x2F); /* CMA opcode */ \
} while (0)

/** ***********************************************************************************************
 * SP <- #d16
 * Loads a 16-bit immediate value into the stack pointer register.
 *************************************************************************************************/
#define LXI_SP(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x31); /* LXI SP,d16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(value)); /* Lower byte of d16 value */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(value)); /* Higher byte of d16 value */ \
} while (0)

/** ***********************************************************************************************
 * [a16] <- A
 * Stores the value of register A into the memory address specified by a16.
 *************************************************************************************************/
#define STA(cpu, offset, addr) do { \
    cpu_write_memory(cpu, offset++, 0x32); /* STA a16 opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(addr)); /* Lower byte of a16 address */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(addr)); /* Higher byte of a16 address */ \
} while (0)

/** ***********************************************************************************************
 * SP <- SP + 1
 * Increments the stack pointer register by 1.
 *************************************************************************************************/
#define INX_SP(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x33); /* INX SP opcode */ \
} while (0)

/** ***********************************************************************************************
 * M <- M + 1
 * Increments the value at the memory address pointed to by the HL register pair by 1.
 * Updates the flags accordingly.
 *************************************************************************************************/
#define INR_M(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x34); /* INR M opcode */ \
} while (0)

/** ***********************************************************************************************
 * M <- M - 1
 * Decrements the value at the memory address pointed to by the HL register pair by 1.
 * Updates the flags accordingly.
 *************************************************************************************************/
#define DCR_M(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x35); /* DCR M opcode */ \
} while (0)

/** ***********************************************************************************************
 * M <- #d8
 * Loads an 8-bit immediate value into the memory address pointed to by the HL register pair.
 *************************************************************************************************/
#define MVI_M(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x36); /* MVI M,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
} while (0)

/** ***********************************************************************************************
 * C <- 1
 * Sets the carry flag to 1.
 *************************************************************************************************/
#define STC(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x37); /* STC opcode */ \
} while (0)

/** ***********************************************************************************************
 * HL <- HL + SP
 * Adds the stack pointer to the HL register pair and updates the carry flag.
 *************************************************************************************************/
#define DAD_SP(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x39); /* DAD SP opcode */ \ \
} while (0)

/** ***********************************************************************************************
 * Halts the CPU until an interrupt is received.
 *************************************************************************************************/
#define HLT(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x76); /* HLT opcode */ \
} while (0)

/** ***********************************************************************************************
 * A <- B
 * Moves the value from register B to register A.
 *************************************************************************************************/
#define MOV_A_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x78); /* MOV A,B opcode */ \
} while (0)

/** ***********************************************************************************************
 * A <- A + B
 * Adds the value of register B to register A and updates the flags accordingly.
 *************************************************************************************************/
#define ADD_B(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0x80); /* ADD B opcode */ \
} while (0)

/** ***********************************************************************************************
 * Calls the restart routine at address 0x0000 and pushes the return address onto the stack.
 *************************************************************************************************/
#define RST_0(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0xC7); /* RST 0 opcode */ \
} while (0)

/** ***********************************************************************************************
 * Returns from a subroutine by popping the return address from the stack and jumping to it.
 *************************************************************************************************/
#define RET(cpu, offset) do { \
    cpu_write_memory(cpu, offset++, 0xC9); /* RET opcode */ \
} while (0)

/** ***********************************************************************************************
 * Calls a subroutine at the specified address and pushes the return address onto the stack.
 *************************************************************************************************/
#define CALL_ADDR(cpu, offset, addr) do { \
    cpu_write_memory(cpu, offset++, 0xCD); /* CALL addr opcode */ \
    cpu_write_memory(cpu, offset++, LOW_BYTE(addr)); /* Lower byte of address */ \
    cpu_write_memory(cpu, offset++, HIGH_BYTE(addr)); /* Upper byte of address */ \
} while (0)

int main(void) {
    CPU cpu = { 0 };
    Bus bus = { 0 };

    bus_allocate_memory(&bus, 256); // Allocate 255 bytes of memory for the bus

    cpu_connect_bus(&cpu, &bus);
    cpu_reset(&cpu);

    uint16_t offset = 0x0000;

    // move 10 into B
    MVI_B(&cpu, offset, 10);
    // move b into a
    MOV_A_B(&cpu, offset);
    // mov 20 into b
    MVI_B(&cpu, offset, 20);
    // add b to a
    ADD_B(&cpu, offset);

    // Load 0x0010 into BC
    LXI_B(&cpu, offset, 0x0020);
    // Store A (which is 30) at address 0x0020
    STAX_B(&cpu, offset);

    // Decrement BC to point to 0x001F)
    DCX_B(&cpu, offset);

    // halt the program
    HLT(&cpu, offset);




    cpu_run(&cpu);
    cpu_log_registers(&cpu);
    bus_dump_memory(&bus);

    (void)getchar();

    bus_free_memory(&bus);
    return 0;
}
