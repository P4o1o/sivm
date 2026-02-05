#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

#include "macros.h"
#include <stdint.h>

/* ==========================================
 * High-Resolution Time
 * 
 * Cross-platform monotonic time for measurements.
 * 
 * Requires: macros.h for platform detection
 * ========================================== */

/* ==========================================
 * PLATFORM INCLUDES
 * ========================================== */
#if defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    #include <mach/mach_time.h>
#elif defined(POSIX)
    #include <time.h>
    #include <unistd.h>
#elif defined(PLATFORM_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
#elif defined(PLATFORM_ZEPHYR)
    #include <zephyr/kernel.h>
#else
    #warning "Unknown platform. Assuming POSIX-like time functions."
    #include <time.h>
    #include <unistd.h>
#endif

/* ==========================================
 * TIME FUNCTIONS
 * ========================================== */

/* Get current monotonic time in platform-specific units */
static force_inline uint64_t time_now(void) {
#if defined(PLATFORM_WINDOWS)
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (uint64_t)li.QuadPart;
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    return mach_absolute_time();
#elif defined(PLATFORM_FREERTOS)
    return (uint64_t)xTaskGetTickCount() * (1000000000ULL / configTICK_RATE_HZ);
#elif defined(PLATFORM_ZEPHYR)
    return k_uptime_ticks() * (1000000000ULL / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
#else  /* POSIX */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

/* Get elapsed nanoseconds between two time points */
static force_inline uint64_t get_time_diff(uint64_t start, uint64_t end) {
#if defined(PLATFORM_WINDOWS)
    static uint64_t freq = 0;
    if (freq == 0) {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        freq = (uint64_t)li.QuadPart;
    }
    return (end - start) * 1000000000ULL / freq;
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    static mach_timebase_info_data_t info = {0};
    if (info.denom == 0) mach_timebase_info(&info);
    return (end - start) * info.numer / info.denom;
#else  /* POSIX, FreeRTOS, Zephyr - already in nanoseconds */
    return end - start;
#endif
}

/* ==========================================
 * SLEEP FUNCTIONS
 * ========================================== */

/* Sleep for milliseconds */
static force_inline void sleep_ms(uint32_t ms) {
#if defined(PLATFORM_WINDOWS)
    Sleep(ms);
#elif defined(PLATFORM_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(PLATFORM_ZEPHYR)
    k_msleep(ms);
#else  /* POSIX */
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
#endif
}

/* Sleep for microseconds */
static force_inline void sleep_us(uint32_t us) {
#if defined(PLATFORM_WINDOWS)
    if (us < 1000) {
        /* Busy wait for sub-millisecond precision on Windows */
        uint64_t start = time_now();
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        uint64_t target = (uint64_t)us * freq.QuadPart / 1000000ULL;
        while (time_now() - start < target) {
            #if SIMD_SSE2
                _mm_pause();
            #endif
        }
    } else {
        Sleep(us / 1000);
    }
#elif defined(PLATFORM_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(us / 1000));
#elif defined(PLATFORM_ZEPHYR)
    k_usleep(us);
#else  /* POSIX */
    struct timespec ts = { .tv_sec = us / 1000000, .tv_nsec = (us % 1000000) * 1000L };
    nanosleep(&ts, NULL);
#endif
}

/* Busy wait for very short delays (nanoseconds) */
static force_inline void spin_ns(uint32_t ns) {
    uint64_t start = time_now();
    while (get_time_diff(start, time_now()) < ns) {
#if SIMD_SSE2 || SIMD_AVX || SIMD_AVX512F
        #if defined(COMPILER_MSVC)
            _mm_pause();
        #else
            __builtin_ia32_pause();
        #endif
#elif SIMD_NEON || defined(ARCH_ARM32) || defined(ARCH_ARM64)
        #if defined(COMPILER_MSVC)
            __yield();
        #else
            __asm__ __volatile__("yield");
        #endif
#else
        /* No pause hint available */
        (void)0;
#endif
    }
}

#endif /* TIMING_H_INCLUDED */
