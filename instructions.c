#include "instructions.h"
#include "cpu.h"

#ifdef _MSC_VER
    #include <intrin.h>
    #define POPCNT(x) __popcnt((unsigned int)(x))
#else
    #define POPCNT(x) __builtin_popcount((unsigned int)(x))
#endif

#define UNUSED_PARAM(x) (void)(x)

void _handler_nop(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(cpu);
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    /* NOP does nothing */
}

void _handler_lxi_b_d16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->B.value = HIGH_BYTE(d16); // Move the higher byte into register B
    cpu->C.value = LOW_BYTE(d16);  // Move the lower byte into register C
}

void _handler_stax_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the address from BC
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the address in BC
}

void _handler_inx_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC
    bc = (bc + 1) & 0xFFFF; // Increment BC and wrap around at 16 bits
    cpu->B.value = HIGH_BYTE(bc); // Update register B with the new higher byte
    cpu->C.value = LOW_BYTE(bc);  // Update register C with the new lower byte
}

void _handler_inr_b(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_b(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_b_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->B.value = d8; // Move the value into register B
}

void _handler_rlc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t msb = (cpu->A.value & 0x80) >> 7; // Get the most significant bit
    
    cpu->A.value = ((cpu->A.value << 1) | msb) & 0xFF; // Rotate left and wrap around
    cpu->flags.f.carry = msb; // Set the carry flag to the original MSB
}

void _handler_dad_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC

    const uint32_t result = hl + bc; // Add BC to HL

    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void _handler_ldax_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the address from BC
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the address in BC into A
}

void _handler_dcx_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint16_t bc = MAKE_WORD(cpu->B.value, cpu->C.value); // Get the current value of BC
    bc = (bc - 1) & 0xFFFF; // Decrement BC and wrap around at 16 bits
    
    cpu->B.value = HIGH_BYTE(bc); // Update register B with the new higher byte
    cpu->C.value = LOW_BYTE(bc);  // Update register C with the new lower byte
}

void _handler_inr_c(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_c(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_c_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->C.value = d8; // Move the value into register C
}

void _handler_rrc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t lsb = cpu->A.value & 0x01; // Get the least significant bit

    cpu->A.value = ((cpu->A.value >> 1) | (lsb << 7)) & 0xFF; // Rotate right and wrap around
    cpu->flags.f.carry = lsb; // Set the carry flag to the original LSB
}

void _handler_lxi_d_d16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->D.value = HIGH_BYTE(d16); // Move the higher byte into register D
    cpu->E.value = LOW_BYTE(d16);  // Move the lower byte into register E
}

void _handler_stax_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the address from DE
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the address in DE
}

void _handler_inx_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    de = (de + 1) & 0xFFFF; // Increment DE and wrap around at 16 bits

    cpu->D.value = HIGH_BYTE(de); // Update register D with the new higher byte
    cpu->E.value = LOW_BYTE(de);  // Update register E with the new lower byte
}

void _handler_inr_d(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_d(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_d_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->D.value = d8; // Move the value into register D
}

void _handler_ral(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t msb = (cpu->A.value & 0x80) >> 7; // Get the most significant bit
    const uint8_t carry_in = cpu->flags.f.carry; // Get the current carry flag

    cpu->A.value = ((cpu->A.value << 1) | carry_in) & 0xFF; // Rotate left and wrap around
    cpu->flags.f.carry = msb; // Set the carry flag to the original MSB
}

void _handler_dad_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    const uint32_t result = hl + de; // Add DE to HL

    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void _handler_ldax_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the address from DE
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the address in DE into A
}

void _handler_dcx_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint16_t de = MAKE_WORD(cpu->D.value, cpu->E.value); // Get the current value of DE
    de = (de - 1) & 0xFFFF; // Decrement DE and wrap around at 16 bits
    
    cpu->D.value = HIGH_BYTE(de); // Update register D with the new higher byte
    cpu->E.value = LOW_BYTE(de);  // Update register E with the new lower byte
}

void _handler_inr_e(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_e(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_e_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->E.value = d8; // Move the value into register E
}

void _handler_rar(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint8_t lsb = cpu->A.value & 0x01; // Get the least significant bit
    const uint8_t carry_in = cpu->flags.f.carry; // Get the current carry flag
    
    cpu->A.value = ((cpu->A.value >> 1) | (carry_in << 7)) & 0xFF; // Rotate right and wrap around
    cpu->flags.f.carry = lsb; // Set the carry flag to the original LSB
}

void _handler_lxi_h_d16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value

    cpu->H.value = HIGH_BYTE(d16); // Move the higher byte into register H
    cpu->L.value = LOW_BYTE(d16);  // Move the lower byte into register L
}

void _handler_shld_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction
    
    cpu_write_memory(cpu, addr, cpu->L.value); // Store the value of L at the lower byte of the address
    cpu_write_memory(cpu, addr + 1, cpu->H.value); // Store the value of H at the higher byte of the address
}

void _handler_inx_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    hl = (hl + 1) & 0xFFFF; // Increment HL and wrap around at 16 bits
    
    cpu->H.value = HIGH_BYTE(hl); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl);  // Update register L with the new lower byte
}

void _handler_inr_h(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_h(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_h_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->H.value = d8; // Move the value into register H
}

void _handler_daa(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dad_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t hl_result = hl + hl; // Add HL to itself (HL * 2)
    
    cpu->H.value = HIGH_BYTE(hl_result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl_result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (hl_result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void _handler_lhld_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
    
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction

    cpu->L.value = cpu_read_memory(cpu, addr); // Load the value at the lower byte of the address into L
    cpu->H.value = cpu_read_memory(cpu, addr + 1); // Load the value at the higher byte of the address into H
}

void _handler_dcx_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    uint16_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    hl = (hl - 1) & 0xFFFF; // Decrement HL and wrap around at 16 bits

    cpu->H.value = HIGH_BYTE(hl); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(hl);  // Update register L with the new lower byte
}

void _handler_inr_l(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_l(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_l_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->L.value = d8; // Move the value into register L
}

void _handler_cma(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = ~cpu->A.value; // Complement the value in register A
}

void _handler_lxi_sp_d16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint8_t d8_low = cpu_read_memory(cpu, curr_pc + 1); // Read the lower byte
    const uint8_t d8_high = cpu_read_memory(cpu, curr_pc + 2); // Read the higher byte
    const uint16_t d16 = MAKE_WORD(d8_high, d8_low); // Combine to form the 16-bit value
    
    cpu->SP.value = d16; // Load the 16-bit value into the stack pointer
}

void _handler_sta_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction
    
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the specified address
}

void _handler_inx_sp(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    cpu->SP.value = (cpu->SP.value + 1) & 0xFFFF; // Increment the stack pointer and wrap around at 16 bits
}

void _handler_inr_m(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_dcr_m(CPU* cpu, uint16_t curr_pc, void* value) {
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

void _handler_mvi_m_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    
    cpu_write_memory(cpu, addr, d8); // Move the value into memory at the address in HL
}

void _handler_stc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->flags.f.carry = 1; // Set the carry flag
}

void _handler_dad_sp(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    uint32_t hl = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the current value of HL
    uint32_t sp = cpu->SP.value; // Get the current value of SP
    const uint32_t result = hl + sp; // Add SP to HL
    
    cpu->H.value = HIGH_BYTE(result); // Update register H with the new higher byte
    cpu->L.value = LOW_BYTE(result);  // Update register L with the new lower byte
    cpu->flags.f.carry = (result > 0xFFFF) ? 1 : 0; // Set carry flag if result exceeds 16 bits
}

void _handler_lda_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);
 
    const uint16_t addr = MAKE_WORD(cpu_read_memory(cpu, curr_pc + 2), cpu_read_memory(cpu, curr_pc + 1)); // Get the 16-bit address from the instruction
    
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the specified address into A
}

void _handler_dcx_sp(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->SP.value = (cpu->SP.value - 1) & 0xFFFF; // Decrement the stack pointer and wrap around at 16 bits
}

void _handler_inr_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->A.value + 1) & 0xFF; // Increment A and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + 1) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = result; // Store the result back in A
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: INR A does not affect the carry flag */
}

void _handler_dcr_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t result = (cpu->A.value - 1) & 0xFF; // Decrement A and wrap around at 8 bits
    
    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) == 0) ? 1 : 0; // Set auxiliary carry flag if borrow from bit 4
    cpu->A.value = result; // Store the result back in A
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (result == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result) & 1); // Set parity flag based on the result
    /* Note: DCR A does not affect the carry flag */
}

void handler_mvi_a_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t d8 = cpu_read_memory(cpu, curr_pc + 1); // Read the immediate value
    cpu->A.value = d8; // Move the value into register A
}

void handler_cmc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->flags.f.carry = !cpu->flags.f.carry; // Complement the carry flag
}

void handler_mov_b_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->B.value; // Move the value from register B to B (no-op)
}

void handler_mov_b_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->C.value; // Move the value from register C to B
}

void handler_mov_b_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->D.value; // Move the value from register D to B
}

void handler_mov_b_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->E.value; // Move the value from register E to B
}

void handler_mov_b_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->H.value; // Move the value from register H to B
}

void handler_mov_b_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->L.value; // Move the value from register L to B
}

void handler_mov_b_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->B.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into B
}

void handler_mov_b_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->B.value = cpu->A.value; // Move the value from register A to B
}

void handler_mov_c_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->B.value; // Move the value from register B to C
}

void handler_mov_c_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->C.value; // Move the value from register C to C (no-op)
}

void handler_mov_c_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->D.value; // Move the value from register D to C
}

void handler_mov_c_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->E.value; // Move the value from register E to C
}

void handler_mov_c_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->H.value; // Move the value from register H to C
}

void handler_mov_c_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->L.value; // Move the value from register L to C
}

void handler_mov_c_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->C.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into C
}

void handler_mov_c_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu->A.value; // Move the value from register A to C
}

void handler_mov_d_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->B.value; // Move the value from register B to D
}

void handler_mov_d_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->C.value; // Move the value from register C to D
}

void handler_mov_d_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->D.value; // Move the value from register D to D (no-op)
}

void handler_mov_d_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->E.value; // Move the value from register E to D
}

void handler_mov_d_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->H.value; // Move the value from register H to D
}

void handler_mov_d_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->L.value; // Move the value from register L to D
}

void handler_mov_d_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->D.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into D
}

void handler_mov_d_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->D.value = cpu->A.value; // Move the value from register A to D
}

void handler_mov_e_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->B.value; // Move the value from register B to E
}

void handler_mov_e_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->C.value; // Move the value from register C to E
}

void handler_mov_e_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->D.value; // Move the value from register D to E
}

void handler_mov_e_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->E.value; // Move the value from register E to E (no-op)
}

void handler_mov_e_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->H.value; // Move the value from register H to E
}

void handler_mov_e_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->L.value; // Move the value from register L to E
}

void handler_mov_e_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->E.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into E
}

void handler_mov_e_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu->A.value; // Move the value from register A to E
}

void handler_mov_h_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->B.value; // Move the value from register B to H
}

void handler_mov_h_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->C.value; // Move the value from register C to H
}

void handler_mov_h_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->D.value; // Move the value from register D to H
}

void handler_mov_h_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->E.value; // Move the value from register E to H
}

void handler_mov_h_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->H.value; // Move the value from register H to H (no-op)
}

void handler_mov_h_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->L.value; // Move the value from register L to H
}

void handler_mov_h_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->H.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into H
}

void handler_mov_h_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->H.value = cpu->A.value; // Move the value from register A to H
}

void handler_mov_l_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->B.value; // Move the value from register B to L
}

void handler_mov_l_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->C.value; // Move the value from register C to L
}

void handler_mov_l_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->D.value; // Move the value from register D to L
}

void handler_mov_l_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->E.value; // Move the value from register E to L
}

void handler_mov_l_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->H.value; // Move the value from register H to L
}

void handler_mov_l_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->L.value; // Move the value from register L to L (no-op)
}

void handler_mov_l_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->L.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into L
}

void handler_mov_l_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->L.value = cpu->A.value; // Move the value from register A to L
}

void handler_mov_m_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->B.value); // Store the value of B at the address in HL
}

void handler_mov_m_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->C.value); // Store the value of C at the address in HL
}

void handler_mov_m_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->D.value); // Store the value of D at the address in HL
}

void handler_mov_m_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->E.value); // Store the value of E at the address in HL
}

void handler_mov_m_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->H.value); // Store the value of H at the address in HL
}

void handler_mov_m_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->L.value); // Store the value of L at the address in HL
}

void handler_hlt(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->halted = 1; // Mark the CPU as halted
}

void handler_mov_m_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu_write_memory(cpu, addr, cpu->A.value); // Store the value of A at the address in HL
}

void handler_mov_a_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->B.value; // Move the value from register B to A
}

void handler_mov_a_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->C.value; // Move the value from register C to A
}

void handler_mov_a_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->D.value; // Move the value from register D to A
}

void handler_mov_a_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->E.value; // Move the value from register E to A
}

void handler_mov_a_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->H.value; // Move the value from register H to A
}

void handler_mov_a_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->L.value; // Move the value from register L to A
}

void handler_mov_a_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    cpu->A.value = cpu_read_memory(cpu, addr); // Load the value at the address in HL into A
}

void handler_mov_a_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value = cpu->A.value; // Move the value from register A to A (no-op)
}

void handler_add_b(CPU* cpu, uint16_t curr_pc, void* value) {
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

void handler_add_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->C.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->C.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->D.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->D.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->E.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->E.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->H.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->H.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->L.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->L.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    const uint16_t result = cpu->A.value + m_value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (m_value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_add_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->A.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->A.value & 0x0F)) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->B.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->B.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->C.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->C.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->D.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->D.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->E.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->E.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->H.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->H.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->L.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->L.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    const uint16_t result = cpu->A.value + m_value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (m_value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_adc_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value + cpu->A.value + cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) + (cpu->A.value & 0x0F) + cpu->flags.f.carry) > 0x0F ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits
}

void handler_sub_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->B.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->B.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->C.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->C.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->D.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->D.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->E.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->E.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->H.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->H.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->L.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->L.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    const uint16_t result = cpu->A.value - m_value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (m_value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sub_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->A.value;

    cpu->flags.f.auxiliary_carry = 0; // No auxiliary carry for A - A
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A (should be 0)
    cpu->flags.f.sign = 0; // Result is zero, so sign flag is cleared
    cpu->flags.f.zero = 1; // Set zero flag since the result is zero
    cpu->flags.f.parity = 1; // Set parity flag since the result has even parity (0)
    cpu->flags.f.carry = 0; // No carry for A - A
}

void handler_sbb_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->B.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->B.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->C.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->C.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->D.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->D.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->E.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->E.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->H.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->H.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->L.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((cpu->L.value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    const uint16_t result = cpu->A.value - m_value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < ((m_value & 0x0F) + cpu->flags.f.carry)) ? 1 : 0; // Set auxiliary carry flag
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A
    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (result > 0xFF) ? 1 : 0; // Set carry flag if result exceeds 8 bits (borrow)
}

void handler_sbb_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->A.value - cpu->flags.f.carry;

    cpu->flags.f.auxiliary_carry = 0; // No auxiliary carry for A - A
    cpu->A.value = (uint8_t)(result & 0xFF); // Store the result back in A (should be 0)
    cpu->flags.f.sign = 0; // Result is zero, so sign flag is cleared
    cpu->flags.f.zero = 1; // Set zero flag since the result is zero
    cpu->flags.f.parity = 1; // Set parity flag since the result has even parity (0)
    cpu->flags.f.carry = 0; // No carry for A - A
}

void handler_ana_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->B.value; // Perform bitwise AND between A and B

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->C.value; // Perform bitwise AND between A and C

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->D.value; // Perform bitwise AND between A and D

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->E.value; // Perform bitwise AND between A and E

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->H.value; // Perform bitwise AND between A and H

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->L.value; // Perform bitwise AND between A and L

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    cpu->A.value &= m_value; // Perform bitwise AND between A and the value at memory[HL]

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_ana_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value &= cpu->A.value; // Perform bitwise AND between A and A (no change)

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ANA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ANA operation
}

void handler_xra_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->B.value; // Perform bitwise XOR between A and B

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->C.value; // Perform bitwise XOR between A and C

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->D.value; // Perform bitwise XOR between A and D

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->E.value; // Perform bitwise XOR between A and E

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->H.value; // Perform bitwise XOR between A and H

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->L.value; // Perform bitwise XOR between A and L

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    cpu->A.value ^= m_value; // Perform bitwise XOR between A and the value at memory[HL]

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_xra_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value ^= cpu->A.value; // Perform bitwise XOR between A and A (result will be 0)

    cpu->flags.f.sign = 0; // Result is zero, so sign flag is cleared
    cpu->flags.f.zero = 1; // Set zero flag since the result is zero
    cpu->flags.f.parity = 1; // Set parity flag since the result has even parity (0)
    cpu->flags.f.carry = 0; // Clear carry flag for XRA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for XRA operation
}

void handler_ora_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->B.value; // Perform bitwise OR between A and B

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->C.value; // Perform bitwise OR between A and C

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->D.value; // Perform bitwise OR between A and D

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->E.value; // Perform bitwise OR between A and E

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->H.value; // Perform bitwise OR between A and H

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->L.value; // Perform bitwise OR between A and L

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    cpu->A.value |= m_value; // Perform bitwise OR between A and the value at memory[HL]

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_ora_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->A.value |= cpu->A.value; // Perform bitwise OR between A and A (no change)

    cpu->flags.f.sign = (cpu->A.value & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = (cpu->A.value == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(cpu->A.value) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = 0; // Clear carry flag for ORA operation
    cpu->flags.f.auxiliary_carry = 0; // Clear auxiliary carry flag for ORA operation
}

void handler_cmp_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->B.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->B.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->B.value) ? 1 : 0; // Set carry flag if A < B
}

void handler_cmp_c(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->C.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->C.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->C.value) ? 1 : 0; // Set carry flag if A < C
}

void handler_cmp_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->D.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->D.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->D.value) ? 1 : 0; // Set carry flag if A < D
}

void handler_cmp_e(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->E.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->E.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->E.value) ? 1 : 0; // Set carry flag if A < E
}

void handler_cmp_h(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->H.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->H.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->H.value) ? 1 : 0; // Set carry flag if A < H
}

void handler_cmp_l(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->L.value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (cpu->L.value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < cpu->L.value) ? 1 : 0; // Set carry flag if A < L
}

void handler_cmp_m(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t addr = MAKE_WORD(cpu->H.value, cpu->L.value); // Get the address from HL
    const uint8_t m_value = cpu_read_memory(cpu, addr); // Read the value at the address in HL

    const uint16_t result = cpu->A.value - m_value;

    cpu->flags.f.auxiliary_carry = ((cpu->A.value & 0x0F) < (m_value & 0x0F)) ? 1 : 0; // Set auxiliary carry flag
    cpu->flags.f.sign = (result & 0x80) ? 1 : 0; // Set sign flag if the result is negative
    cpu->flags.f.zero = ((uint8_t)(result & 0xFF) == 0) ? 1 : 0; // Set zero flag if the result is zero
    cpu->flags.f.parity = !(POPCNT(result & 0xFF) & 1); // Set parity flag based on the result
    cpu->flags.f.carry = (cpu->A.value < m_value) ? 1 : 0; // Set carry flag if A < [HL]
}

void handler_cmp_a(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint16_t result = cpu->A.value - cpu->A.value;

    cpu->flags.f.auxiliary_carry = 0; // no half-borrow when subtracting equal values
    cpu->flags.f.sign = 0; // result = 0x00
    cpu->flags.f.zero = 1; // result is zero
    cpu->flags.f.parity = 1; // parity(0x00) = even
    cpu->flags.f.carry = 0; // no borrow when A == A
}

void handler_rnz(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // If Zero flag is set, do nothing (no return)
    if (cpu->flags.f.zero != 0) {
        return;
    }

    // Pop return address from stack
    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value + 1);

    cpu->SP.value += 2;
    cpu->PC.value = (uint16_t)(hi << 8) | lo;
}

void handler_pop_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->C.value = cpu_read_memory(cpu, cpu->SP.value);
    cpu->B.value = cpu_read_memory(cpu, cpu->SP.value + 1);
    cpu->SP.value += 2;
}

void handler_jnz_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.zero != 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    cpu->PC.value = addr;
}

void handler_jmp_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    uint16_t addr = MAKE_WORD(hi, lo);

    cpu->PC.value = addr;
}

void handler_cnz_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.zero != 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    // Compute return address (next instruction)
    const uint16_t ret = curr_pc + 3;

    // Push return address (high byte first, stack grows downward)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, (ret >> 8) & 0xFF);

    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, ret & 0xFF);

    // Jump to target
    cpu->PC.value = addr;
}

void handler_push_b(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // Push high byte (B)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, cpu->B.value);

    // Push low byte (C)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, cpu->C.value);
}

void handler_adi_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t imm = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t A = cpu->A.value;
    const uint16_t result = (uint16_t)A + (uint16_t)imm;

    // Update accumulator
    cpu->A.value = (uint8_t)(result & 0xFF);

    // Flags
    cpu->flags.f.carry = (result > 0xFF);
    cpu->flags.f.auxiliary_carry = ((A & 0x0F) + (imm & 0x0F)) > 0x0F;
    cpu->flags.f.zero = (cpu->A.value == 0);
    cpu->flags.f.sign = (cpu->A.value & 0x80) != 0;
    cpu->flags.f.parity = (POPCNT(cpu->A.value) % 2 == 0);
}

void handler_rst_0(CPU* cpu, uint16_t curr_pc, void* value) {
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

void handler_rz(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // If Zero flag is not set, do nothing (no return)
    if (cpu->flags.f.zero == 0) {
        return;
    }

    // Pop return address from stack
    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value + 1);

    cpu->SP.value += 2;
    cpu->PC.value = (uint16_t)(hi << 8) | lo;
}

void handler_ret(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    cpu->SP.value += 1;

    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value);
    cpu->SP.value += 1;

    cpu->PC.value = MAKE_WORD(hi, lo); /* Jump back to the return address */
}

void handler_jz_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.zero == 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    cpu->PC.value = addr;
}

void handler_cz_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.zero == 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    // Compute return address (next instruction)
    const uint16_t ret = curr_pc + 3;

    // Push return address (high byte first, stack grows downward)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, (ret >> 8) & 0xFF);

    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, ret & 0xFF);

    // Jump to target
    cpu->PC.value = addr;
}

void handler_call_a16(CPU* cpu, uint16_t curr_pc, void* value) {
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

void handler_aci_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t imm = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t A = cpu->A.value;
    const uint8_t cy = cpu->flags.f.carry ? 1 : 0;
    const uint16_t result = (uint16_t)A + (uint16_t)imm + (uint16_t)cy;

    // Update accumulator
    cpu->A.value = (uint8_t)(result & 0xFF);

    // Flags
    cpu->flags.f.carry = (result > 0xFF);
    cpu->flags.f.auxiliary_carry = ((A & 0x0F) + (imm & 0x0F) + cy) > 0x0F;
    cpu->flags.f.zero = (cpu->A.value == 0);
    cpu->flags.f.sign = (cpu->A.value & 0x80) != 0;
    cpu->flags.f.parity = (POPCNT(cpu->A.value) % 2 == 0);
}

void handler_rst_1(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint16_t return_address = curr_pc + 1; /* address after RST instruction */
    const uint8_t hi = HIGH_BYTE(return_address);
    const uint8_t lo = LOW_BYTE(return_address);

    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, hi);
    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, lo);

    cpu->PC.value = 0x0008; /* Jump to the restart address (0x0008 for RST 1) */
}

void handler_rnc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // If carry flag is set, do nothing (no return)
    if (cpu->flags.f.carry != 0) {
        return;
    }

    // Pop return address from stack
    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value + 1);

    cpu->SP.value += 2;
    cpu->PC.value = (uint16_t)(hi << 8) | lo;
}

void handler_pop_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    cpu->E.value = cpu_read_memory(cpu, cpu->SP.value);
    cpu->D.value = cpu_read_memory(cpu, cpu->SP.value + 1);
    cpu->SP.value += 2;
}

void handler_jnc_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.carry != 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    cpu->PC.value = addr;
}

void handler_out_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    // Read port number
    const uint8_t port = cpu_read_memory(cpu, curr_pc + 1);

    // Write A to the port
    cpu_write_port(cpu, port, cpu->A.value);
}

void handler_cnc_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.carry != 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    // Compute return address (next instruction)
    const uint16_t ret = curr_pc + 3;

    // Push return address (high byte first, stack grows downward)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, (ret >> 8) & 0xFF);

    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, ret & 0xFF);

    // Jump to target
    cpu->PC.value = addr;
}

void handler_push_d(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // Push high byte (D)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, cpu->D.value);

    // Push low byte (E)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, cpu->E.value);
}

void handler_sui_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t imm = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t A = cpu->A.value;
    const uint16_t result = (uint16_t)A - (uint16_t)imm;

    // Update accumulator
    cpu->A.value = (uint8_t)(result & 0xFF);

    // Flags
    cpu->flags.f.carry = A < imm;
    cpu->flags.f.auxiliary_carry = (A & 0x0F) < (imm & 0x0F);
    cpu->flags.f.zero = (cpu->A.value == 0);
    cpu->flags.f.sign = (cpu->A.value & 0x80) != 0;
    cpu->flags.f.parity = (POPCNT(cpu->A.value) % 2 == 0);
}

void handler_rst_2(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint16_t return_address = curr_pc + 1; /* address after RST instruction */
    const uint8_t hi = HIGH_BYTE(return_address);
    const uint8_t lo = LOW_BYTE(return_address);

    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, hi);
    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, lo);

    cpu->PC.value = 0x0010; /* Jump to the restart address (0x0010 for RST 2) */
}

void handler_rc(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(curr_pc);
    UNUSED_PARAM(value);

    // If Carry flag is not set, do nothing (no return)
    if (cpu->flags.f.carry == 0) {
        return;
    }

    // Pop return address from stack
    const uint8_t lo = cpu_read_memory(cpu, cpu->SP.value);
    const uint8_t hi = cpu_read_memory(cpu, cpu->SP.value + 1);

    cpu->SP.value += 2;
    cpu->PC.value = (uint16_t)(hi << 8) | lo;
}

void handler_jc_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.carry == 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    cpu->PC.value = addr;
}

void handler_in_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    // Read port number
    uint8_t port = cpu_read_memory(cpu, curr_pc + 1);

    // Read from port into A
    cpu->A.value = cpu_read_port(cpu, port);
}

void handler_cc_a16(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    if (cpu->flags.f.carry == 0) {
        cpu->PC.value = curr_pc + 3;
        return;
    }

    const uint8_t lo = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t hi = cpu_read_memory(cpu, curr_pc + 2);
    const uint16_t addr = MAKE_WORD(hi, lo);

    // Compute return address (next instruction)
    const uint16_t ret = curr_pc + 3;

    // Push return address (high byte first, stack grows downward)
    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, (ret >> 8) & 0xFF);

    cpu->SP.value--;
    cpu_write_memory(cpu, cpu->SP.value, ret & 0xFF);

    // Jump to target
    cpu->PC.value = addr;
}

void handler_sbi_d8(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint8_t imm = cpu_read_memory(cpu, curr_pc + 1);
    const uint8_t A = cpu->A.value;
    const uint8_t cy = cpu->flags.f.carry ? 1 : 0;

    // Perform subtraction with borrow
    const uint16_t result = (uint16_t)A - (uint16_t)imm - (uint16_t)cy;

    // Update accumulator
    cpu->A.value = (uint8_t)(result & 0xFF);

    // Flags
    cpu->flags.f.carry = (A < (uint16_t)imm + cy); // borrow
    cpu->flags.f.auxiliary_carry = ((A & 0x0F) < ((imm & 0x0F) + cy)); // half-borrow
    cpu->flags.f.zero = (cpu->A.value == 0);
    cpu->flags.f.sign = (cpu->A.value & 0x80) != 0;
    cpu->flags.f.parity = (POPCNT(cpu->A.value) % 2 == 0);
}

void handler_rst_3(CPU* cpu, uint16_t curr_pc, void* value) {
    UNUSED_PARAM(value);

    const uint16_t return_address = curr_pc + 1; /* address after RST instruction */
    const uint8_t hi = HIGH_BYTE(return_address);
    const uint8_t lo = LOW_BYTE(return_address);

    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, hi);
    cpu->SP.value -= 1;
    cpu_write_memory(cpu, cpu->SP.value, lo);

    cpu->PC.value = 0x0018; /* Jump to the restart address (0x0018 for RST 3) */
}



const Instruction instructions[256] = {
    [0x00] = { 0x00, 1,  4, _handler_nop }, /* NOP */
    [0x01] = { 0x01, 3, 10, _handler_lxi_b_d16 }, /* LXI B,d16 */
    [0x02] = { 0x02, 1,  7, _handler_stax_b }, /* STAX B */
    [0x03] = { 0x03, 1,  5, _handler_inx_b }, /* INX B */
    [0x04] = { 0x04, 1,  5, _handler_inr_b }, /* INR B */
    [0x05] = { 0x05, 1,  5, _handler_dcr_b }, /* DCR B */
    [0x06] = { 0x06, 2,  7, _handler_mvi_b_d8 }, /* MVI B,d8 */
    [0x07] = { 0x07, 1,  4, _handler_rlc }, /* RLC */
    [0x08] = { 0x08, 1,  4, _handler_nop }, /* NOP */
    [0x09] = { 0x09, 1, 10, _handler_dad_b }, /* DAD B */
    [0x0A] = { 0x0A, 1,  7, _handler_ldax_b }, /* LDAX B */
    [0x0B] = { 0x0B, 1,  5, _handler_dcx_b }, /* DCX B */
    [0x0C] = { 0x0C, 1,  5, _handler_inr_c }, /* INR C */
    [0x0D] = { 0x0D, 1,  5, _handler_dcr_c }, /* DCR C */
    [0x0E] = { 0x0E, 2,  7, _handler_mvi_c_d8 }, /* MVI C,d8 */
    [0x0F] = { 0x0F, 1,  4, _handler_rrc }, /* RRC */

    [0x10] = { 0x10, 1,  4, _handler_nop }, /* NOP */
    [0x11] = { 0x11, 3, 10, _handler_lxi_d_d16 }, /* LXI D,d16 */
    [0x12] = { 0x12, 1,  7, _handler_stax_d }, /* STAX D */
    [0x13] = { 0x13, 1,  5, _handler_inx_d }, /* INX D */
    [0x14] = { 0x14, 1,  5, _handler_inr_d }, /* INR D */
    [0x15] = { 0x15, 1,  5, _handler_dcr_d }, /* DCR D */
    [0x16] = { 0x16, 2,  7, _handler_mvi_d_d8 }, /* MVI D,d8 */
    [0x17] = { 0x17, 1,  4, _handler_ral }, /* RAL */
    [0x18] = { 0x18, 1,  4, _handler_nop }, /* NOP */
    [0x19] = { 0x19, 1, 10, _handler_dad_d }, /* DAD D */
    [0x1A] = { 0x1A, 1,  7, _handler_ldax_d }, /* LDAX D */
    [0x1B] = { 0x1B, 1,  5, _handler_dcx_d }, /* DCX D */
    [0x1C] = { 0x1C, 1,  5, _handler_inr_e }, /* INR E */
    [0x1D] = { 0x1D, 1,  5, _handler_dcr_e }, /* DCR E */
    [0x1E] = { 0x1E, 2,  7, _handler_mvi_e_d8 }, /* MVI E,d8 */
    [0x1F] = { 0x1F, 1,  4, _handler_rar }, /* RAR */

    [0x20] = { 0x20, 1,  4, _handler_nop }, /* NOP */
    [0x21] = { 0x21, 3, 10, _handler_lxi_h_d16 }, /* LXI H,d16 */
    [0x22] = { 0x22, 3, 16, _handler_shld_a16 }, /* SHLD a16 */
    [0x23] = { 0x23, 1,  5, _handler_inx_h }, /* INX H */
    [0x24] = { 0x24, 1,  5, _handler_inr_h }, /* INR H */
    [0x25] = { 0x25, 1,  5, _handler_dcr_h }, /* DCR H */
    [0x26] = { 0x26, 2,  7, _handler_mvi_h_d8 }, /* MVI H,d8 */
    [0x27] = { 0x27, 1,  4, _handler_daa }, /* DAA */
    [0x28] = { 0x28, 1,  4, _handler_nop }, /* NOP */
    [0x29] = { 0x29, 1, 10, _handler_dad_h }, /* DAD H */
    [0x2A] = { 0x2A, 3, 16, _handler_lhld_a16 }, /* LHLD a16 */
    [0x2B] = { 0x2B, 1,  5, _handler_dcx_h }, /* DCX H */
    [0x2C] = { 0x2C, 1,  5, _handler_inr_l }, /* INR L */
    [0x2D] = { 0x2D, 1,  5, _handler_dcr_l }, /* DCR L */
    [0x2E] = { 0x2E, 2,  7, _handler_mvi_l_d8 }, /* MVI L,d8 */
    [0x2F] = { 0x2F, 1,  4, _handler_cma }, /* CMA */

    [0x30] = { 0x30, 1,  4, _handler_nop }, /* NOP */
    [0x31] = { 0x31, 3, 10, _handler_lxi_sp_d16 }, /* LXI SP,d16 */
    [0x32] = { 0x32, 3, 13, _handler_sta_a16 }, /* STA a16 */
    [0x33] = { 0x33, 1,  5, _handler_inx_sp }, /* INX SP */
    [0x34] = { 0x34, 1, 10, _handler_inr_m }, /* INR M */
    [0x35] = { 0x35, 1, 10, _handler_dcr_m }, /* DCR M */
    [0x36] = { 0x36, 2, 10, _handler_mvi_m_d8 }, /* MVI M,d8 */
    [0x37] = { 0x37, 1,  4, _handler_stc }, /* STC */
    [0x38] = { 0x38, 1,  4, _handler_nop }, /* NOP */
    [0x39] = { 0x39, 1, 10, _handler_dad_sp }, /* DAD SP */
    [0x3A] = { 0x3A, 3, 13, _handler_lda_a16 }, /* LDA a16 */
    [0x3B] = { 0x3B, 1,  5, _handler_dcx_sp }, /* DCX SP */
    [0x3C] = { 0x3C, 1,  5, _handler_inr_a }, /* INR A */
    [0x3D] = { 0x3D, 1,  5, _handler_dcr_a }, /* DCR A */
    [0x3E] = { 0x3E, 2,  7, handler_mvi_a_d8 }, /* MVI A,d8 */
    [0x3F] = { 0x3F, 1,  4, handler_cmc }, /* CMC */

    [0x40] = { 0x40, 1,  5, handler_mov_b_b }, /* MOV B,B */
    [0x41] = { 0x41, 1,  5, handler_mov_b_c }, /* MOV B,C */
    [0x42] = { 0x42, 1,  5, handler_mov_b_d }, /* MOV B,D */
    [0x43] = { 0x43, 1,  5, handler_mov_b_e }, /* MOV B,E */
    [0x44] = { 0x44, 1,  5, handler_mov_b_h }, /* MOV B,H */
    [0x45] = { 0x45, 1,  5, handler_mov_b_l }, /* MOV B,L */
    [0x46] = { 0x46, 1,  7, handler_mov_b_m }, /* MOV B,M */
    [0x47] = { 0x47, 1,  5, handler_mov_b_a }, /* MOV B,A */
    [0x48] = { 0x48, 1,  5, handler_mov_c_b }, /* MOV C,B */
    [0x49] = { 0x49, 1,  5, handler_mov_c_c }, /* MOV C,C */
    [0x4A] = { 0x4A, 1,  5, handler_mov_c_d }, /* MOV C,D */
    [0x4B] = { 0x4B, 1,  5, handler_mov_c_e }, /* MOV C,E */
    [0x4C] = { 0x4C, 1,  5, handler_mov_c_h }, /* MOV C,H */
    [0x4D] = { 0x4D, 1,  5, handler_mov_c_l }, /* MOV C,L */
    [0x4E] = { 0x4E, 1,  7, handler_mov_c_m }, /* MOV C,M */
    [0x4F] = { 0x4F, 1,  5, handler_mov_c_a }, /* MOV C,A */

    [0x50] = { 0x50, 1,  5, handler_mov_d_b }, /* MOV D,B */
    [0x51] = { 0x51, 1,  5, handler_mov_d_c }, /* MOV D,C */
    [0x52] = { 0x52, 1,  5, handler_mov_d_d }, /* MOV D,D */
    [0x53] = { 0x53, 1,  5, handler_mov_d_e }, /* MOV D,E */
    [0x54] = { 0x54, 1,  5, handler_mov_d_h }, /* MOV D,H */
    [0x55] = { 0x55, 1,  5, handler_mov_d_l }, /* MOV D,L */
    [0x56] = { 0x56, 1,  7, handler_mov_d_m }, /* MOV D,M */
    [0x57] = { 0x57, 1,  5, handler_mov_d_a }, /* MOV D,A */
    [0x58] = { 0x58, 1,  5, handler_mov_e_b }, /* MOV E,B */
    [0x59] = { 0x59, 1,  5, handler_mov_e_c }, /* MOV E,C */
    [0x5A] = { 0x5A, 1,  5, handler_mov_e_d }, /* MOV E,D */
    [0x5B] = { 0x5B, 1,  5, handler_mov_e_e }, /* MOV E,E */
    [0x5C] = { 0x5C, 1,  5, handler_mov_e_h }, /* MOV E,H */
    [0x5D] = { 0x5D, 1,  5, handler_mov_e_l }, /* MOV E,L */
    [0x5E] = { 0x5E, 1,  7, handler_mov_e_m }, /* MOV E,M */
    [0x5F] = { 0x5F, 1,  5, handler_mov_e_a }, /* MOV E,A */

    [0x60] = { 0x60, 1,  5, handler_mov_h_b }, /* MOV H,B */
    [0x61] = { 0x61, 1,  5, handler_mov_h_c }, /* MOV H,C */
    [0x62] = { 0x62, 1,  5, handler_mov_h_d }, /* MOV H,D */
    [0x63] = { 0x63, 1,  5, handler_mov_h_e }, /* MOV H,E */
    [0x64] = { 0x64, 1,  5, handler_mov_h_h }, /* MOV H,H */
    [0x65] = { 0x65, 1,  5, handler_mov_h_l }, /* MOV H,L */
    [0x66] = { 0x66, 1,  7, handler_mov_h_m }, /* MOV H,M */
    [0x67] = { 0x67, 1,  5, handler_mov_h_a }, /* MOV H,A */
    [0x68] = { 0x68, 1,  5, handler_mov_l_b }, /* MOV L,B */
    [0x69] = { 0x69, 1,  5, handler_mov_l_c }, /* MOV L,C */
    [0x6A] = { 0x6A, 1,  5, handler_mov_l_d }, /* MOV L,D */
    [0x6B] = { 0x6B, 1,  5, handler_mov_l_e }, /* MOV L,E */
    [0x6C] = { 0x6C, 1,  5, handler_mov_l_h }, /* MOV L,H */
    [0x6D] = { 0x6D, 1,  5, handler_mov_l_l }, /* MOV L,L */
    [0x6E] = { 0x6E, 1,  7, handler_mov_l_m }, /* MOV L,M */
    [0x6F] = { 0x6F, 1,  5, handler_mov_l_a }, /* MOV L,A */

    [0x70] = { 0x70, 1,  7, handler_mov_m_b }, /* MOV M,B */
    [0x71] = { 0x71, 1,  7, handler_mov_m_c }, /* MOV M,C */
    [0x72] = { 0x72, 1,  7, handler_mov_m_d }, /* MOV M,D */
    [0x73] = { 0x73, 1,  7, handler_mov_m_e }, /* MOV M,E */
    [0x74] = { 0x74, 1,  7, handler_mov_m_h }, /* MOV M,H */
    [0x75] = { 0x75, 1,  7, handler_mov_m_l }, /* MOV M,L */
    [0x76] = { 0x76, 1,  7, handler_hlt }, /* HLT */
    [0x77] = { 0x77, 1,  7, handler_mov_m_a }, /* MOV M,A */
    [0x78] = { 0x78, 1,  5, handler_mov_a_b }, /* MOV A,B */
    [0x79] = { 0x79, 1,  5, handler_mov_a_c }, /* MOV A,C */
    [0x7A] = { 0x7A, 1,  5, handler_mov_a_d }, /* MOV A,D */
    [0x7B] = { 0x7B, 1,  5, handler_mov_a_e }, /* MOV A,E */
    [0x7C] = { 0x7C, 1,  5, handler_mov_a_h }, /* MOV A,H */
    [0x7D] = { 0x7D, 1,  5, handler_mov_a_l }, /* MOV A,L */
    [0x7E] = { 0x7E, 1,  7, handler_mov_a_m }, /* MOV A,M */
    [0x7F] = { 0x7F, 1,  5, handler_mov_a_a }, /* MOV A,A */

    [0x80] = { 0x80, 1,  4, handler_add_b }, /* ADD B */
    [0x81] = { 0x81, 1,  4, handler_add_c }, /* ADD C */
    [0x82] = { 0x82, 1,  4, handler_add_d }, /* ADD D */
    [0x83] = { 0x83, 1,  4, handler_add_e }, /* ADD E */
    [0x84] = { 0x84, 1,  4, handler_add_h }, /* ADD H */
    [0x85] = { 0x85, 1,  4, handler_add_l }, /* ADD L */
    [0x86] = { 0x86, 1,  7, handler_add_m }, /* ADD M */
    [0x87] = { 0x87, 1,  4, handler_add_a }, /* ADD A */
    [0x88] = { 0x88, 1,  4, handler_adc_b }, /* ADC B */
    [0x89] = { 0x89, 1,  4, handler_adc_c }, /* ADC C */
    [0x8A] = { 0x8A, 1,  4, handler_adc_d }, /* ADC D */
    [0x8B] = { 0x8B, 1,  4, handler_adc_e }, /* ADC E */
    [0x8C] = { 0x8C, 1,  4, handler_adc_h }, /* ADC H */
    [0x8D] = { 0x8D, 1,  4, handler_adc_l }, /* ADC L */
    [0x8E] = { 0x8E, 1,  7, handler_adc_m }, /* ADC M */
    [0x8F] = { 0x8F, 1,  4, handler_adc_a }, /* ADC A */

    [0x90] = { 0x90, 1,  4, handler_sub_b }, /* SUB B */
    [0x91] = { 0x91, 1,  4, handler_sub_c }, /* SUB C */
    [0x92] = { 0x92, 1,  4, handler_sub_d }, /* SUB D */
    [0x93] = { 0x93, 1,  4, handler_sub_e }, /* SUB E */
    [0x94] = { 0x94, 1,  4, handler_sub_h }, /* SUB H */
    [0x95] = { 0x95, 1,  4, handler_sub_l }, /* SUB L */
    [0x96] = { 0x96, 1,  7, handler_sub_m }, /* SUB M */
    [0x97] = { 0x97, 1,  4, handler_sub_a }, /* SUB A */
    [0x98] = { 0x98, 1,  4, handler_sbb_b }, /* SBB B */
    [0x99] = { 0x99, 1,  4, handler_sbb_c }, /* SBB C */
    [0x9A] = { 0x9A, 1,  4, handler_sbb_d }, /* SBB D */
    [0x9B] = { 0x9B, 1,  4, handler_sbb_e }, /* SBB E */
    [0x9C] = { 0x9C, 1,  4, handler_sbb_h }, /* SBB H */
    [0x9D] = { 0x9D, 1,  4, handler_sbb_l }, /* SBB L */
    [0x9E] = { 0x9E, 1,  7, handler_sbb_m }, /* SBB M */
    [0x9F] = { 0x9F, 1,  4, handler_sbb_a }, /* SBB A */

    [0xA0] = { 0xA0, 1,  4, handler_ana_b }, /* ANA B */
    [0xA1] = { 0xA1, 1,  4, handler_ana_c }, /* ANA C */
    [0xA2] = { 0xA2, 1,  4, handler_ana_d }, /* ANA D */
    [0xA3] = { 0xA3, 1,  4, handler_ana_e }, /* ANA E */
    [0xA4] = { 0xA4, 1,  4, handler_ana_h }, /* ANA H */
    [0xA5] = { 0xA5, 1,  4, handler_ana_l }, /* ANA L */
    [0xA6] = { 0xA6, 1,  7, handler_ana_m }, /* ANA M */
    [0xA7] = { 0xA7, 1,  4, handler_ana_a }, /* ANA A */
    [0xA8] = { 0xA8, 1,  4, handler_xra_b }, /* XRA B */
    [0xA9] = { 0xA9, 1,  4, handler_xra_c }, /* XRA C */
    [0xAA] = { 0xAA, 1,  4, handler_xra_d }, /* XRA D */
    [0xAB] = { 0xAB, 1,  4, handler_xra_e }, /* XRA E */
    [0xAC] = { 0xAC, 1,  4, handler_xra_h }, /* XRA H */
    [0xAD] = { 0xAD, 1,  4, handler_xra_l }, /* XRA L */
    [0xAE] = { 0xAE, 1,  7, handler_xra_m }, /* XRA M */
    [0xAF] = { 0xAF, 1,  4, handler_xra_a }, /* XRA A */

    [0xB0] = { 0xB0, 1,  4, handler_ora_b }, /* ORA B */
    [0xB1] = { 0xB1, 1,  4, handler_ora_c }, /* ORA C */
    [0xB2] = { 0xB2, 1,  4, handler_ora_d }, /* ORA D */
    [0xB3] = { 0xB3, 1,  4, handler_ora_e }, /* ORA E */
    [0xB4] = { 0xB4, 1,  4, handler_ora_h }, /* ORA H */
    [0xB5] = { 0xB5, 1,  4, handler_ora_l }, /* ORA L */
    [0xB6] = { 0xB6, 1,  7, handler_ora_m }, /* ORA M */
    [0xB7] = { 0xB7, 1,  4, handler_ora_a }, /* ORA A */
    [0xB8] = { 0xB8, 1,  4, handler_cmp_b }, /* CMP B */
    [0xB9] = { 0xB9, 1,  4, handler_cmp_c }, /* CMP C */
    [0xBA] = { 0xBA, 1,  4, handler_cmp_d }, /* CMP D */
    [0xBB] = { 0xBB, 1,  4, handler_cmp_e }, /* CMP E */
    [0xBC] = { 0xBC, 1,  4, handler_cmp_h }, /* CMP H */
    [0xBD] = { 0xBD, 1,  4, handler_cmp_l }, /* CMP L */
    [0xBE] = { 0xBE, 1,  7, handler_cmp_m }, /* CMP M */
    [0xBF] = { 0xBF, 1,  4, handler_cmp_a }, /* CMP A */

    [0xC0] = { 0xC0, 1, (11 << 8) | 5, handler_rnz }, /* RNZ */
    [0xC1] = { 0xC1, 1, 10, handler_pop_b }, /* POP B */
    [0xC2] = { 0xC2, 3, 10, handler_jnz_a16 }, /* JNZ a16 */
    [0xC3] = { 0xC3, 3, 10, handler_jmp_a16 }, /* JMP a16 */
    [0xC4] = { 0xC4, 3, (17 << 8) | 11, handler_cnz_a16 }, /* CNZ a16 */
    [0xC5] = { 0xC5, 1, 11, handler_push_b }, /* PUSH B */
    [0xC6] = { 0xC6, 2, 7, handler_adi_d8 }, /* ADI d8 */
    [0xC7] = { 0xC7, 1, 11, handler_rst_0 }, /* RST 0 */
    [0xC8] = { 0xC8, 1, (11 << 8) | 5, handler_rz }, /* RZ */
    [0xC9] = { 0xC9, 1, 10, handler_ret }, /* RET */
    [0xCA] = { 0xCA, 3, 10, handler_jz_a16 }, /* JZ a16 */
    [0xCB] = { 0xCB, 3, 10, handler_jmp_a16 }, /* JMP a16 */
    [0xCC] = { 0xCC, 3, (17 << 8) | 11, handler_cz_a16 }, /* CZ a16 */
    [0xCD] = { 0xCD, 3, 17, handler_call_a16 }, /* CALL a16 */
    [0xCE] = { 0xCE, 2, 7, handler_aci_d8 }, /* ACI d8 */
    [0xCF] = { 0xCF, 1, 11, handler_rst_1 }, /* RST 1 */

    [0xD0] = { 0xD0, 1, (11 << 8) | 5, handler_rnc }, /* RNC */
    [0xD1] = { 0xD1, 1, 10, handler_pop_d }, /* POP D */
    [0xD2] = { 0xD2, 3, 10, handler_jnc_a16 }, /* JNC a16 */
    [0xD3] = { 0xD3, 2, 10, handler_out_d8 }, /* OUT d8 */
    [0xD4] = { 0xD4, 3, (17 << 8) | 11, handler_cnc_a16 }, /* CNC a16 */
    [0xD5] = { 0xD5, 1, 11, handler_push_d }, /* PUSH D */
    [0xD6] = { 0xD6, 2, 7, handler_sui_d8 }, /* SUI d8 */
    [0xD7] = { 0xD7, 1, 11, handler_rst_2 }, /* RST 2 */
    [0xD8] = { 0xD8, 1, (11 < 8) | 5, handler_rc }, /* RC */
    [0xD9] = { 0xD9, 1, 10, handler_ret }, /* RET */
    [0xDA] = { 0xDA, 3, 10, handler_jc_a16 }, /* JC a16 */
    [0xDB] = { 0xDB, 2, 10, handler_in_d8 }, /* IN d8 */
    [0xDC] = { 0xDC, 3, (17 << 8) | 11, handler_cc_a16 }, /* CC a16 */
    [0xDD] = { 0xDD, 3, 17, handler_call_a16 }, /* CALL a16 */
    [0xDE] = { 0xDE, 2, 7, handler_sbi_d8 }, /* SBI d8 */
    [0xDF] = { 0xDF, 1, 11, handler_rst_3 }, /* RST 3 */


};
