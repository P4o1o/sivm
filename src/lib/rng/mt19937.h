#ifndef MT19937_H_INCLUDED
#define MT19937_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "../macros.h"

/* ==========================================
 * Mersenne Twister 19937
 * 
 * High-quality pseudorandom number generator.
 * SIMD-accelerated for batch generation.
 * 
 * Supported SIMD:
 * - x86/x64: AVX-512, AVX2, SSE2
 * - ARM: NEON
 * - PowerPC: VSX
 * 
 * Requires: macros.h for SIMD detection
 * ========================================== */

#define MT19937_STATE_SIZE ((size_t) 624) 

struct MT19937{
#if SIMD_AVX512F
    alignas(64) uint32_t state[MT19937_STATE_SIZE + 13];
#elif SIMD_AVX2 || SIMD_AVX
    alignas(32) uint32_t state[MT19937_STATE_SIZE + 5];
#elif SIMD_SSE2 || SIMD_NEON
    alignas(16) uint32_t state[MT19937_STATE_SIZE + 1];
#else
    uint32_t state[MT19937_STATE_SIZE];
#endif
    size_t next;
};

void init_MT19937(const uint32_t seed, struct MT19937 * rand_engine);
uint32_t get_MT19937(struct MT19937 * rand_engine);

#if SIMD_AVX512F
    __m512i get_16_MT19937(struct MT19937 * rand_engine);
#endif

#if SIMD_AVX2 || SIMD_AVX512F
    __m256i get_8_MT19937(struct MT19937 * rand_engine);
#endif

#if SIMD_SSE2 || SIMD_AVX2 || SIMD_AVX512F
    __m128i get_4_MT19937(struct MT19937 * rand_engine);
#elif SIMD_NEON
    uint32x4_t get_4_MT19937(struct MT19937 * rand_engine);
#endif

#define MT19937_64_STATE_SIZE ((size_t) 312) 

struct MT19937_64{
#if SIMD_AVX512F
    alignas(64) uint64_t state[MT19937_64_STATE_SIZE + 4];
#elif SIMD_AVX2 || SIMD_AVX
    alignas(32) uint64_t state[MT19937_64_STATE_SIZE + 1];
#elif SIMD_SSE2 || SIMD_NEON
    alignas(16) uint64_t state[MT19937_64_STATE_SIZE + 1];
#else
    uint64_t state[MT19937_64_STATE_SIZE];
#endif
    size_t next;
};

void init_MT19937_64(const uint64_t seed, struct MT19937_64 * rand_engine);
uint64_t get_MT19937_64(struct MT19937_64 * rand_engine);

#if SIMD_AVX512F
    __m512i get_8_MT19937_64(struct MT19937_64 * rand_engine);
#endif

#if SIMD_AVX2 || SIMD_AVX512F
    __m256i get_4_MT19937_64(struct MT19937_64 * rand_engine);
#endif

#if SIMD_SSE2 || SIMD_AVX2 || SIMD_AVX512F
    __m128i get_2_MT19937_64(struct MT19937_64 * rand_engine);
#elif SIMD_NEON
    uint64x2_t get_2_MT19937_64(struct MT19937_64 * rand_engine);
#endif

#endif /* MT19937_H_INCLUDED */
