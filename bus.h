#ifndef BUS_H
#define BUS_H

#include <stdint.h>

typedef struct Memory {
    size_t size;
    size_t capacity;
    uint8_t* data;
} Memory;

typedef struct BUS {
    Memory memory;
} BUS;

void bus_allocate_memory(BUS* bus, size_t capacity);
void bus_free_memory(BUS* bus);
void bus_dump_memory_range(const BUS* bus, uint16_t start, uint16_t end);
void bus_dump_memory(const BUS* bus);
uint8_t bus_read(const BUS* bus, uint16_t address);
void bus_write(BUS* bus, uint16_t address, uint8_t value);

#endif
