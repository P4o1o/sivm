#include "prob.h"

/* ==========================================
 * Thread-local RNG state definitions
 * 
 * Initialized per-thread on first use.
 * ========================================== */

#if defined(PROB_RNG_MT19937)
    THREAD_LOCAL struct MT19937 random_engine;
    THREAD_LOCAL uint32_t random_ready = 0;

#elif defined(PROB_RNG_MT19937_64)
    THREAD_LOCAL struct MT19937_64 random_engine_64;
    THREAD_LOCAL uint32_t random_ready = 0;

#elif defined(PROB_RNG_XORSHIFT)
    THREAD_LOCAL xorshift128plus_t xorshift_state = {{0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL}};
    THREAD_LOCAL uint32_t random_ready = 0;

#elif defined(PROB_RNG_STDLIB)
    THREAD_LOCAL uint32_t random_ready = 0;

#endif
