#include "timing.h"
#include "cpu.h"

#include <Windows.h>

static LARGE_INTEGER g_perf_freq;
static LARGE_INTEGER g_start_time;
static uint8_t g_timer_ok = 0;

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
