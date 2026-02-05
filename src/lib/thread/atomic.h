#ifndef ATOMIC_H_INCLUDED
#define ATOMIC_H_INCLUDED

#include "../macros.h"
#include <stdint.h>

/* ==========================================
 * Atomic Operations
 * 
 * Lock-free primitives for concurrent programming.
 * Wraps compiler intrinsics for maximum performance.
 * 
 * Requires: macros.h for compiler/architecture detection
 * ========================================== */

/* ==========================================
 * MEMORY ORDERING
 * ========================================== */

#define MEMORY_ORDER_RELAXED 0
#define MEMORY_ORDER_CONSUME 1
#define MEMORY_ORDER_ACQUIRE 2
#define MEMORY_ORDER_RELEASE 3
#define MEMORY_ORDER_ACQ_REL 4
#define MEMORY_ORDER_SEQ_CST 5

/* ==========================================
 * COMPILER-SPECIFIC DEFINITIONS
 * ========================================== */

#if defined(COMPILER_MSVC)
    #include <intrin.h>

    /* Memory barriers */
    static force_inline void atomic_fence_acquire(void) { _ReadBarrier(); _mm_lfence(); }
    static force_inline void atomic_fence_release(void) { _WriteBarrier(); _mm_sfence(); }
    static force_inline void atomic_fence_seq_cst(void) { _ReadWriteBarrier(); _mm_mfence(); }
    static force_inline void atomic_fence(int order) {
        switch (order) {
            case MEMORY_ORDER_ACQUIRE: _ReadBarrier(); _mm_lfence(); break;
            case MEMORY_ORDER_RELEASE: _WriteBarrier(); _mm_sfence(); break;
            case MEMORY_ORDER_ACQ_REL:
            case MEMORY_ORDER_SEQ_CST: _ReadWriteBarrier(); _mm_mfence(); break;
            default: break;
        }
    }
    static force_inline void atomic_signal_fence(int order) { _ReadWriteBarrier(); (void)order; }

    /* Base operations: load, store, exchange, compare_exchange */
    #define ATOMIC_OP_PROTOTYPE(type, bits, suffix, cast_type) \
        static force_inline type##bits##_t atomic_load_##type##bits(const volatile type##bits##_t* ptr) { \
            type##bits##_t val = *ptr; \
            _ReadBarrier(); \
            return val; \
        } \
        static force_inline type##bits##_t atomic_load_##type##bits##_explicit(const volatile type##bits##_t* ptr, int order) { \
            type##bits##_t val = *ptr; \
            if (order >= MEMORY_ORDER_ACQUIRE) _ReadBarrier(); \
            return val; \
        } \
        static force_inline void atomic_store_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            _WriteBarrier(); \
            *ptr = val; \
            _mm_mfence(); \
        } \
        static force_inline void atomic_store_##type##bits##_explicit(volatile type##bits##_t* ptr, type##bits##_t val, int order) { \
            if (order >= MEMORY_ORDER_RELEASE) _WriteBarrier(); \
            *ptr = val; \
            if (order == MEMORY_ORDER_SEQ_CST) _mm_mfence(); \
        } \
        static force_inline type##bits##_t atomic_exchange_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedExchange##suffix((volatile cast_type*)ptr, (cast_type)val); \
        } \
        static force_inline int atomic_compare_exchange_##type##bits(volatile type##bits##_t* ptr, type##bits##_t* expected, type##bits##_t desired) { \
            type##bits##_t old = (type##bits##_t)_InterlockedCompareExchange##suffix((volatile cast_type*)ptr, (cast_type)desired, (cast_type)*expected); \
            if (old == *expected) return 1; \
            *expected = old; \
            return 0; \
        } \
        static force_inline int atomic_compare_exchange_weak_##type##bits(volatile type##bits##_t* ptr, type##bits##_t* expected, type##bits##_t desired) { \
            return atomic_compare_exchange_##type##bits(ptr, expected, desired); \
        }

    /* Integer-only: arithmetic and bitwise */
    #define ATOMIC_OP_PROTOTYPE_INTEGERS(type, bits, suffix, cast_type) \
        ATOMIC_OP_PROTOTYPE(type, bits, suffix, cast_type) \
        static force_inline type##bits##_t atomic_fetch_add_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedExchangeAdd##suffix((volatile cast_type*)ptr, (cast_type)val); \
        } \
        static force_inline type##bits##_t atomic_fetch_sub_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedExchangeAdd##suffix((volatile cast_type*)ptr, -(cast_type)val); \
        } \
        static force_inline type##bits##_t atomic_fetch_and_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedAnd##suffix((volatile cast_type*)ptr, (cast_type)val); \
        } \
        static force_inline type##bits##_t atomic_fetch_or_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedOr##suffix((volatile cast_type*)ptr, (cast_type)val); \
        } \
        static force_inline type##bits##_t atomic_fetch_xor_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return (type##bits##_t)_InterlockedXor##suffix((volatile cast_type*)ptr, (cast_type)val); \
        } \
        static force_inline type##bits##_t atomic_fetch_inc_##type##bits(volatile type##bits##_t* ptr) { \
            return (type##bits##_t)_InterlockedExchangeAdd##suffix((volatile cast_type*)ptr, 1); \
        } \
        static force_inline type##bits##_t atomic_fetch_dec_##type##bits(volatile type##bits##_t* ptr) { \
            return (type##bits##_t)_InterlockedExchangeAdd##suffix((volatile cast_type*)ptr, -1); \
        } \
        static force_inline type##bits##_t atomic_inc_##type##bits(volatile type##bits##_t* ptr) { \
            return (type##bits##_t)_InterlockedIncrement##suffix((volatile cast_type*)ptr); \
        } \
        static force_inline type##bits##_t atomic_dec_##type##bits(volatile type##bits##_t* ptr) { \
            return (type##bits##_t)_InterlockedDecrement##suffix((volatile cast_type*)ptr); \
        }

#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    /* GCC/Clang built-in atomics */

    /* Memory barriers */
    static force_inline void atomic_fence_acquire(void) { __atomic_thread_fence(__ATOMIC_ACQUIRE); }
    static force_inline void atomic_fence_release(void) { __atomic_thread_fence(__ATOMIC_RELEASE); }
    static force_inline void atomic_fence_seq_cst(void) { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
    static force_inline void atomic_fence(int order) { __atomic_thread_fence(order); }
    static force_inline void atomic_signal_fence(int order) { __atomic_signal_fence(order); }

    /* Base operations: load, store, exchange, compare_exchange */
    #define ATOMIC_OP_PROTOTYPE(type, bits, suffix, cast_type) \
        static force_inline type##bits##_t atomic_load_##type##bits(const volatile type##bits##_t* ptr) { \
            return __atomic_load_n(ptr, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_load_##type##bits##_explicit(const volatile type##bits##_t* ptr, int order) { \
            return __atomic_load_n(ptr, order); \
        } \
        static force_inline void atomic_store_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline void atomic_store_##type##bits##_explicit(volatile type##bits##_t* ptr, type##bits##_t val, int order) { \
            __atomic_store_n(ptr, val, order); \
        } \
        static force_inline type##bits##_t atomic_exchange_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline int atomic_compare_exchange_##type##bits(volatile type##bits##_t* ptr, type##bits##_t* expected, type##bits##_t desired) { \
            return __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        } \
        static force_inline int atomic_compare_exchange_weak_##type##bits(volatile type##bits##_t* ptr, type##bits##_t* expected, type##bits##_t desired) { \
            return __atomic_compare_exchange_n(ptr, expected, desired, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
        }

    /* Integer-only: arithmetic and bitwise */
    #define ATOMIC_OP_PROTOTYPE_INTEGERS(type, bits, suffix, cast_type) \
        ATOMIC_OP_PROTOTYPE(type, bits, suffix, cast_type) \
        static force_inline type##bits##_t atomic_fetch_add_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_sub_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_and_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_or_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_xor_##type##bits(volatile type##bits##_t* ptr, type##bits##_t val) { \
            return __atomic_fetch_xor(ptr, val, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_inc_##type##bits(volatile type##bits##_t* ptr) { \
            return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_fetch_dec_##type##bits(volatile type##bits##_t* ptr) { \
            return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_inc_##type##bits(volatile type##bits##_t* ptr) { \
            return __atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST); \
        } \
        static force_inline type##bits##_t atomic_dec_##type##bits(volatile type##bits##_t* ptr) { \
            return __atomic_sub_fetch(ptr, 1, __ATOMIC_SEQ_CST); \
        }

#else
    #error "Unsupported compiler for atomic operations. Define COMPILER_MSVC, COMPILER_GCC, or COMPILER_CLANG."
#endif /* Compiler selection */

    /* ==========================================
    * INSTANTIATION
    * MSVC: suffix empty for 32-bit, "64" for 64-bit
    * GCC: suffix and cast_type ignored
    * ========================================== */

    ATOMIC_OP_PROTOTYPE_INTEGERS(uint, 32, , long)
    ATOMIC_OP_PROTOTYPE_INTEGERS(uint, 64, 64, __int64)
    ATOMIC_OP_PROTOTYPE_INTEGERS(int, 32, , long)
    ATOMIC_OP_PROTOTYPE_INTEGERS(int, 64, 64, __int64)

    /* ==========================================
 * POINTER ATOMIC OPERATIONS
 * ========================================== */

#if ARCH_64BIT
    /* 64-bit architectures */
    static force_inline void* atomic_load_ptr(void* const volatile* ptr) {
        return (void*)atomic_load_uint64((const volatile uint64_t*)ptr);
    }
    static force_inline void* atomic_load_ptr_explicit(void* const volatile* ptr, int order) {
        return (void*)atomic_load_uint64_explicit((const volatile uint64_t*)ptr, order);
    }
    static force_inline void atomic_store_ptr(void* volatile* ptr, void* val) {
        atomic_store_uint64((volatile uint64_t*)ptr, (uint64_t)val);
    }
    static force_inline void atomic_store_ptr_explicit(void* volatile* ptr, void* val, int order) {
        atomic_store_uint64_explicit((volatile uint64_t*)ptr, (uint64_t)val, order);
    }
    static force_inline void* atomic_exchange_ptr(void* volatile* ptr, void* val) {
        return (void*)atomic_exchange_uint64((volatile uint64_t*)ptr, (uint64_t)val);
    }
    static force_inline int atomic_compare_exchange_ptr(void* volatile* ptr, void** expected, void* desired) {
        return atomic_compare_exchange_uint64((volatile uint64_t*)ptr, (uint64_t*)expected, (uint64_t)desired);
    }
#else
    /* 32-bit architectures */
    static force_inline void* atomic_load_ptr(void* const volatile* ptr) {
        return (void*)(uintptr_t)atomic_load_uint32((const volatile uint32_t*)ptr);
    }
    static force_inline void* atomic_load_ptr_explicit(void* const volatile* ptr, int order) {
        return (void*)(uintptr_t)atomic_load_uint32_explicit((const volatile uint32_t*)ptr, order);
    }
    static force_inline void atomic_store_ptr(void* volatile* ptr, void* val) {
        atomic_store_uint32((volatile uint32_t*)ptr, (uint32_t)(uintptr_t)val);
    }
    static force_inline void atomic_store_ptr_explicit(void* volatile* ptr, void* val, int order) {
        atomic_store_uint32_explicit((volatile uint32_t*)ptr, (uint32_t)(uintptr_t)val, order);
    }
    static force_inline void* atomic_exchange_ptr(void* volatile* ptr, void* val) {
        return (void*)(uintptr_t)atomic_exchange_uint32((volatile uint32_t*)ptr, (uint32_t)(uintptr_t)val);
    }
    static force_inline int atomic_compare_exchange_ptr(void* volatile* ptr, void** expected, void* desired) {
        return atomic_compare_exchange_uint32((volatile uint32_t*)ptr, (uint32_t*)expected, (uint32_t)(uintptr_t)desired);
    }
#endif

/* ==========================================
 * SIZE_T ATOMIC OPERATIONS
 * ========================================== */

#if ARCH_64BIT
    /* 64-bit architectures */
    static force_inline size_t atomic_load_size(const volatile size_t* ptr) { return (size_t)atomic_load_uint64((const volatile uint64_t*)ptr); }
    static force_inline void atomic_store_size(volatile size_t* ptr, size_t val) { atomic_store_uint64((volatile uint64_t*)ptr, (uint64_t)val); }
    static force_inline size_t atomic_exchange_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_exchange_uint64((volatile uint64_t*)ptr, (uint64_t)val); }
    static force_inline size_t atomic_fetch_add_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_fetch_add_uint64((volatile uint64_t*)ptr, (uint64_t)val); }
    static force_inline size_t atomic_fetch_sub_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_fetch_sub_uint64((volatile uint64_t*)ptr, (uint64_t)val); }
    static force_inline size_t atomic_inc_size(volatile size_t* ptr) { return (size_t)atomic_inc_uint64((volatile uint64_t*)ptr); }
    static force_inline size_t atomic_dec_size(volatile size_t* ptr) { return (size_t)atomic_dec_uint64((volatile uint64_t*)ptr); }
#else
    /* 32-bit architectures */
    static force_inline size_t atomic_load_size(const volatile size_t* ptr) { return (size_t)atomic_load_uint32((const volatile uint32_t*)ptr); }
    static force_inline void atomic_store_size(volatile size_t* ptr, size_t val) { atomic_store_uint32((volatile uint32_t*)ptr, (uint32_t)val); }
    static force_inline size_t atomic_exchange_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_exchange_uint32((volatile uint32_t*)ptr, (uint32_t)val); }
    static force_inline size_t atomic_fetch_add_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_fetch_add_uint32((volatile uint32_t*)ptr, (uint32_t)val); }
    static force_inline size_t atomic_fetch_sub_size(volatile size_t* ptr, size_t val) { return (size_t)atomic_fetch_sub_uint32((volatile uint32_t*)ptr, (uint32_t)val); }
    static force_inline size_t atomic_inc_size(volatile size_t* ptr) { return (size_t)atomic_inc_uint32((volatile uint32_t*)ptr); }
    static force_inline size_t atomic_dec_size(volatile size_t* ptr) { return (size_t)atomic_dec_uint32((volatile uint32_t*)ptr); }
#endif

/* ==========================================
 * CPU PAUSE HINT (for spin loops)
 * 
 * Note: cpu_pause() is already defined as a macro in macros.h
 * with proper platform/architecture support. Use that instead.
 * ========================================== */

/* ==========================================
 * SPINLOCK
 * ========================================== */

#define SPINLOCK_PROTOTYPE(type, bits) \
    typedef volatile type##bits##_t spinlock_##type##bits; \
    static force_inline void spinlock_##type##bits##_lock(spinlock_##type##bits* lock) { \
        while (atomic_exchange_##type##bits(lock, 1)) { \
            while (atomic_load_##type##bits##_explicit(lock, MEMORY_ORDER_RELAXED)) { \
                cpu_pause(); \
            } \
        } \
    } \
    static force_inline int spinlock_##type##bits##_trylock(spinlock_##type##bits* lock) { \
        return atomic_exchange_##type##bits(lock, 1) == 0; \
    } \
    static force_inline void spinlock_##type##bits##_unlock(spinlock_##type##bits* lock) { \
        atomic_store_##type##bits##_explicit(lock, 0, MEMORY_ORDER_RELEASE); \
    }

SPINLOCK_PROTOTYPE(uint, 32)
SPINLOCK_PROTOTYPE(uint, 64)
SPINLOCK_PROTOTYPE(int, 32)
SPINLOCK_PROTOTYPE(int, 64)

/* Default spinlock (32-bit, most cache-friendly) */
typedef spinlock_uint32 spinlock;
#define SPINLOCK_INIT 0
#define spinlock_lock spinlock_uint32_lock
#define spinlock_trylock spinlock_uint32_trylock
#define spinlock_unlock spinlock_uint32_unlock

/* ==========================================
 * ATOMIC FLAG
 * ========================================== */

#define ATOMIC_FLAG_PROTOTYPE(type, bits) \
    typedef volatile type##bits##_t atomic_flag_##type##bits; \
    static force_inline type##bits##_t atomic_flag_##type##bits##_test_and_set(atomic_flag_##type##bits* flag) { \
        return atomic_exchange_##type##bits(flag, 1); \
    } \
    static force_inline void atomic_flag_##type##bits##_clear(atomic_flag_##type##bits* flag) { \
        atomic_store_##type##bits##_explicit(flag, 0, MEMORY_ORDER_RELEASE); \
    } \
    static force_inline type##bits##_t atomic_flag_##type##bits##_load(atomic_flag_##type##bits* flag) { \
        return atomic_load_##type##bits##_explicit(flag, MEMORY_ORDER_ACQUIRE); \
    }

ATOMIC_FLAG_PROTOTYPE(uint, 32)
ATOMIC_FLAG_PROTOTYPE(uint, 64)
ATOMIC_FLAG_PROTOTYPE(int, 32)
ATOMIC_FLAG_PROTOTYPE(int, 64)

/* Default atomic_flag (32-bit) */
typedef atomic_flag_uint32 atomic_flag;
#define ATOMIC_FLAG_INIT 0
#define atomic_flag_test_and_set atomic_flag_uint32_test_and_set
#define atomic_flag_clear atomic_flag_uint32_clear
#define atomic_flag_load atomic_flag_uint32_load

#endif /* ATOMIC_H_INCLUDED */