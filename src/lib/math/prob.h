#ifndef PROB_H_INCLUDED
#define PROB_H_INCLUDED

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <float.h>
#include "../macros.h"

/* ==========================================
 * Probability & Random Utilities
 * 
 * Thread-local random number generation.
 * Configurable RNG backend via macros.
 * 
 * RNG Selection (define before including):
 *   PROB_RNG_MT19937   - Mersenne Twister (default, high quality)
 *   PROB_RNG_MT19937_64 - Mersenne Twister 64-bit
 *   PROB_RNG_XORSHIFT  - XorShift128+ (faster, good quality)
 *   PROB_RNG_STDLIB    - Standard library rand() (fastest, low quality)
 * 
 * Example:
 *   #define PROB_RNG_XORSHIFT
 *   #include "prob.h"
 * 
 * Requires: macros.h
 * ========================================== */

#if HAS_OPENMP
    #include <omp.h>
#endif

/* ==========================================
 * RNG Backend Selection
 * ========================================== */

/* Default to MT19937 if nothing specified */
#if !defined(PROB_RNG_MT19937) && !defined(PROB_RNG_MT19937_64) && \
    !defined(PROB_RNG_XORSHIFT) && !defined(PROB_RNG_STDLIB)
    #define PROB_RNG_MT19937
#endif

/* ==========================================
 * RNG Implementations
 * ========================================== */

#if defined(PROB_RNG_MT19937)
    /* Mersenne Twister 32-bit - High quality, slower */
    #include "../rng/mt19937.h"
    
    extern THREAD_LOCAL struct MT19937 random_engine;
    extern THREAD_LOCAL uint32_t random_ready;
    
    static force_inline uint32_t random32(void) { return get_MT19937(&random_engine); }
    static force_inline void random32_init(uint32_t seed) { init_MT19937(seed, &random_engine); random_ready = 1; }
    static force_inline int is_random32_init(void) { return random_ready; }
    
    #define RANDOM_MAX 0xFFFFFFFFU
    #define PROB_RNG_NAME "MT19937"
    
    /* SIMD batch random generation */
    #if SIMD_AVX512F
    static force_inline __m512i random32_16(void) { return get_16_MT19937(&random_engine); }
    #endif
    #if SIMD_AVX2 || SIMD_AVX512F
    static force_inline __m256i random32_8(void) { return get_8_MT19937(&random_engine); }
    #endif
    #if SIMD_SSE2 || SIMD_AVX2 || SIMD_AVX512F
    static force_inline __m128i random32_4(void) { return get_4_MT19937(&random_engine); }
    #elif SIMD_NEON
    static force_inline uint32x4_t random32_4(void) { return get_4_MT19937(&random_engine); }
    #endif

#elif defined(PROB_RNG_MT19937_64)
    /* Mersenne Twister 64-bit - High quality, native 64-bit */
    #include "../rng/mt19937.h"
    
    extern THREAD_LOCAL struct MT19937_64 random_engine_64;
    extern THREAD_LOCAL uint32_t random_ready;
    
    static force_inline uint64_t random64(void) { return get_MT19937_64(&random_engine_64); }
    static force_inline uint32_t random32(void) { return (uint32_t)get_MT19937_64(&random_engine_64); }
    static force_inline void random32_init(uint32_t seed) { init_MT19937_64((uint64_t)seed, &random_engine_64); random_ready = 1; }
    static force_inline void random64_init(uint64_t seed) { init_MT19937_64(seed, &random_engine_64); random_ready = 1; }
    static force_inline int is_random32_init(void) { return random_ready; }
    
    #define RANDOM_MAX 0xFFFFFFFFU
    #define RANDOM64_MAX 0xFFFFFFFFFFFFFFFFULL
    #define PROB_RNG_NAME "MT19937-64"
    
    /* SIMD batch random generation */
    #if SIMD_AVX512F
    static force_inline __m512i random64_8(void) { return get_8_MT19937_64(&random_engine_64); }
    #endif
    #if SIMD_AVX2 || SIMD_AVX512F
    static force_inline __m256i random64_4(void) { return get_4_MT19937_64(&random_engine_64); }
    #endif
    #if SIMD_SSE2 || SIMD_AVX2 || SIMD_AVX512F
    static force_inline __m128i random64_2(void) { return get_2_MT19937_64(&random_engine_64); }
    #elif SIMD_NEON
    static force_inline uint64x2_t random64_2(void) { return get_2_MT19937_64(&random_engine_64); }
    #endif

#elif defined(PROB_RNG_XORSHIFT)
    /* XorShift128+ - Fast, good quality */
    
    typedef struct {
        uint64_t s[2];
    } xorshift128plus_t;
    
    extern THREAD_LOCAL xorshift128plus_t xorshift_state;
    extern THREAD_LOCAL uint32_t random_ready;
    
    static force_inline uint64_t xorshift128plus_next(xorshift128plus_t* state) {
        uint64_t s1 = state->s[0];
        const uint64_t s0 = state->s[1];
        const uint64_t result = s0 + s1;
        state->s[0] = s0;
        s1 ^= s1 << 23;
        state->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
        return result;
    }
    
    static force_inline uint64_t random64(void) { return xorshift128plus_next(&xorshift_state); }
    static force_inline uint32_t random32(void) { return (uint32_t)xorshift128plus_next(&xorshift_state); }
    
    static force_inline void random32_init(uint32_t seed) {
        /* SplitMix64 to initialize state */
        uint64_t z = (uint64_t)seed;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        xorshift_state.s[0] = z ^ (z >> 31);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        xorshift_state.s[1] = z ^ (z >> 31);
        random_ready = 1;
    }
    
    static force_inline void random64_init(uint64_t seed) {
        uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        xorshift_state.s[0] = z ^ (z >> 31);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        xorshift_state.s[1] = z ^ (z >> 31);
        random_ready = 1;
    }
    
    static force_inline int is_random32_init(void) { return random_ready; }
    
    #define RANDOM_MAX 0xFFFFFFFFU
    #define RANDOM64_MAX 0xFFFFFFFFFFFFFFFFULL
    #define PROB_RNG_NAME "XorShift128+"

#elif defined(PROB_RNG_STDLIB)
    /* Standard library rand() - Fastest, lowest quality */
    
    extern THREAD_LOCAL uint32_t random_ready;
    
    static force_inline uint32_t random32(void) { return (uint32_t)rand(); }
    static force_inline void random32_init(uint32_t seed) { srand(seed); random_ready = 1; }
    static force_inline int is_random32_init(void) { return random_ready; }
    
    #define RANDOM_MAX RAND_MAX
    #define PROB_RNG_NAME "stdlib"
    
#endif

/* ==========================================
 * Probability Types and Macros
 * ========================================== */

typedef uint64_t prob;
#define MAX_PROB (((prob) RANDOM_MAX) + 1)
#define MIN_PROB 0
#define PROB_PRECISION (1.0 / MAX_PROB)
#define PROBABILITY(val) ((prob)(((double) MAX_PROB) * (val)))
#define INVERSE_PROB(p) (MAX_PROB - (p))
#define WILL_HAPPEN(p) ((p) > (prob) random32())

/* ==========================================
 * Random Integer Generation
 * ========================================== */

#define RAND_BOUNDS(min, max) ((min) + ((uint64_t) random32() % ((max) - (min) + (uint64_t) 1)))
#define RAND_UPTO(max) ((uint64_t) random32() % ((max) + (uint64_t) 1))

/* ==========================================
 * Random Floating Point Generation
 * ========================================== */

#define RAND_DOUBLE() (DBL_MIN + ((double)random32() / RANDOM_MAX) * (DBL_MAX - DBL_MIN))
#define RAND_DBL_BOUNDS(min, max) ((min) + ((double)random32() / RANDOM_MAX) * ((max) - (min)))

/* Uniform [0, 1) */
#define RAND_UNIFORM() ((double)random32() / ((double)RANDOM_MAX + 1.0))

/* Uniform [0, 1] */
#define RAND_UNIFORM_CLOSED() ((double)random32() / (double)RANDOM_MAX)

#endif /* PROB_H_INCLUDED */
