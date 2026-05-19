#include "bus.h"

#include <stdlib.h>

void bus_allocate_memory(BUS *bus, size_t capacity) {
    bus->memory.size = 0;
    bus->memory.capacity = capacity;
    bus->memory.data = calloc(capacity, sizeof(uint8_t));
}

void bus_free_memory(BUS* bus) {
    free(bus->memory.data);
    bus->memory.data = NULL;
    bus->memory.size = 0;
    bus->memory.capacity = 0;
}

void bus_dump_memory_range(const BUS* bus, uint16_t start, uint16_t end) {
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

void bus_dump_memory(const BUS* bus) {
    bus_dump_memory_range(bus, 0, (uint16_t)bus->memory.capacity - 1);
}

uint8_t bus_read(const BUS* bus, uint16_t address) {
    if (address < bus->memory.capacity) {
        return bus->memory.data[address];
    }
    return 0;
}

void bus_write(BUS* bus, uint16_t address, uint8_t value) {
    if (address < bus->memory.capacity) {
        bus->memory.data[address] = value;
    }
}
