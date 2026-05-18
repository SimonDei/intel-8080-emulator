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
void mvi_c_d8_handler(CPU* cpu, uint16_t curr_pc, void* value);
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
    [0x0C] = { 0x0C, 1,  5, NULL }, /* INR C */
    [0x0D] = { 0x0D, 1,  5, NULL }, /* DCR C */
    [0x0E] = { 0x0E, 2,  7, mvi_c_d8_handler }, /* MVI C,d8 */
    [0x0F] = { 0x0F, 1,  4, NULL }, /* RRC */
    [0x10] = { 0x10, 1,  4, nop_handler }, /* NOP */
    [0x18] = { 0x18, 1,  4, nop_handler }, /* NOP */
    [0x20] = { 0x20, 1,  4, nop_handler }, /* NOP */
    [0x28] = { 0x28, 1,  4, nop_handler }, /* NOP */
    [0x30] = { 0x30, 1,  4, nop_handler }, /* NOP */
    [0x38] = { 0x38, 1,  4, nop_handler }, /* NOP */
    [0x76] = { 0x76, 1,  7, hlt_handler }, /* HLT */
    [0x78] = { 0x78, 1,  5, mov_a_b_handler }, /* MOV A,B */
    [0x80] = { 0x80, 1,  4, add_b_handler }, /* ADD B */
    [0xC7] = { 0xC7, 1, 11, rst_0_handler }, /* RST 0 */
    [0xC9] = { 0xC9, 1, 10, ret_handler }, /* RET */
    [0xCD] = { 0xCD, 3, 17, call_addr_handler }, /* CALL addr */
    /* ... Fill in the rest of the instructions ... */
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

void mvi_c_d8_handler(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->C.value = d8; // Move the value into register C
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
 * C <- #d8
 * Loads an 8-bit immediate value into the C register.
 *************************************************************************************************/
#define MVI_C(cpu, offset, value) do { \
    cpu_write_memory(cpu, offset++, 0x0E); /* MVI C,d8 opcode */ \
    cpu_write_memory(cpu, offset++, value); /* d8 value */ \
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
