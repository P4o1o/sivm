#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

/* ==========================================
 * Feature Test Macros
 * 
 * Define POSIX/BSD features if not already set by compiler flags.
 * Must be before ANY system headers.
 * ========================================== */
#if !defined(_WIN32) && !defined(__ZEPHYR__) && !defined(ESP_PLATFORM) && !defined(PLATFORM_FREERTOS)
    #if !defined(_POSIX_C_SOURCE)
        #define _POSIX_C_SOURCE 200809L
    #endif
    #if defined(__linux__) && !defined(_GNU_SOURCE)
        #define _GNU_SOURCE
    #endif
    #if (defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)) && !defined(_BSD_SOURCE)
        #define _BSD_SOURCE
    #endif
    #if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
        #define _DARWIN_C_SOURCE
    #endif
#endif

#include <stddef.h>

//      Architecture
/*
     0: x86        (32-bit)
     1: x86_64     (64-bit)
     2: ARM32      (32-bit)
     3: ARM64      (64-bit)
     4: RISC-V 32  (32-bit)
     5: RISC-V 64  (64-bit)
     6: PPC32      (32-bit)
     7: PPC64      (64-bit)
     8: MIPS32     (32-bit)
     9: MIPS64     (64-bit)
    10: SPARC32    (32-bit)
    11: SPARC64    (64-bit)
    12: WASM32     (32-bit)
    13: WASM64     (64-bit)
    14: LoongArch32(32-bit)
    15: LoongArch64(64-bit)
    16: S390X      (64-bit)
    17: SuperH     (32-bit)
    18: m68k       (32-bit)
    19: UNKNOWN
    
    Note: (ARCHITECTURE & 1) == 1 means 64-bit for pairs 0-15
*/
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    #define ARCHITECTURE 1
    #define ARCH_X64 1
    #define ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86) || defined(__i386)
    #define ARCHITECTURE 0
    #define ARCH_X86 1
    #define ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM64__)
    #define ARCHITECTURE 3
    #define ARCH_ARM64 1
    #define ARCH_NAME "ARM64"
#elif defined(__arm__) || defined(_M_ARM) || defined(__ARM_ARCH)
    #define ARCHITECTURE 2
    #define ARCH_ARM32 1
    #define ARCH_NAME "ARM32"
#elif defined(__riscv)
    #if __riscv_xlen == 64
        #define ARCHITECTURE 5
        #define ARCH_RISCV64 1
        #define ARCH_NAME "RISC-V64"
    #else
        #define ARCHITECTURE 4
        #define ARCH_RISCV32 1
        #define ARCH_NAME "RISC-V32"
    #endif
#elif defined(__powerpc64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
    #define ARCHITECTURE 7
    #define ARCH_PPC64 1
    #define ARCH_NAME "PPC64"
#elif defined(__powerpc__) || defined(__ppc__) || defined(_ARCH_PPC)
    #define ARCHITECTURE 6
    #define ARCH_PPC32 1
    #define ARCH_NAME "PPC32"
#elif defined(__mips64) || defined(__mips64__)
    #define ARCHITECTURE 9
    #define ARCH_MIPS64 1
    #define ARCH_NAME "MIPS64"
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__)
    #define ARCHITECTURE 8
    #define ARCH_MIPS32 1
    #define ARCH_NAME "MIPS32"
#elif defined(__sparc64__) || defined(__sparc_v9__)
    #define ARCHITECTURE 11
    #define ARCH_SPARC64 1
    #define ARCH_NAME "SPARC64"
#elif defined(__sparc__) || defined(__sparc)
    #define ARCHITECTURE 10
    #define ARCH_SPARC32 1
    #define ARCH_NAME "SPARC32"
#elif defined(__wasm64__)
    #define ARCHITECTURE 13
    #define ARCH_WASM64 1
    #define ARCH_NAME "WASM64"
#elif defined(__wasm__) || defined(__EMSCRIPTEN__)
    #define ARCHITECTURE 12
    #define ARCH_WASM32 1
    #define ARCH_NAME "WASM32"
#elif defined(__loongarch64)
    #define ARCHITECTURE 15
    #define ARCH_LOONGARCH64 1
    #define ARCH_NAME "LoongArch64"
#elif defined(__loongarch__)
    #define ARCHITECTURE 14
    #define ARCH_LOONGARCH32 1
    #define ARCH_NAME "LoongArch32"
#elif defined(__s390x__) || defined(__zarch__)
    #define ARCHITECTURE 16
    #define ARCH_S390X 1
    #define ARCH_NAME "S390X"
#elif defined(__sh__) || defined(__SH__)
    #define ARCHITECTURE 17
    #define ARCH_SH 1
    #define ARCH_NAME "SuperH"
#elif defined(__m68k__) || defined(__MC68K__)
    #define ARCHITECTURE 18
    #define ARCH_M68K 1
    #define ARCH_NAME "m68k"
#else
    #define ARCHITECTURE 19
    #define ARCH_UNKNOWN 1
    #define ARCH_NAME "Unknown"
#endif

// Architecture bit-width
#if (ARCHITECTURE <= 15 && (ARCHITECTURE & 1)) || ARCHITECTURE == 16
    #define ARCH_64BIT 1
    #define ARCH_BITS 64
#else
    #define ARCH_32BIT 1
    #define ARCH_BITS 32
#endif

//      Endian
/*
    0: little
    1: big
    2: UNKNOWN
*/
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define ENDIAN 0
        #define ENDIAN_LITTLE 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define ENDIAN 1
        #define ENDIAN_BIG 1
    #else
        #define ENDIAN 2
    #endif
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__)
    #define ENDIAN 0
    #define ENDIAN_LITTLE 1
#elif defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || \
      defined(_MIPSEL) || defined(__MIPSEL__) || defined(__riscv) || \
      defined(__wasm__) || defined(__loongarch__)
    #define ENDIAN 0
    #define ENDIAN_LITTLE 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB__) || \
      defined(__sparc__) || defined(__s390__) || defined(__m68k__)
    #define ENDIAN 1
    #define ENDIAN_BIG 1
#else
    #define ENDIAN 2
#endif

// Byte swap macros
#if defined(__GNUC__) || defined(__clang__)
    #define BSWAP16(x) __builtin_bswap16(x)
    #define BSWAP32(x) __builtin_bswap32(x)
    #define BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #include <stdlib.h>
    #define BSWAP16(x) _byteswap_ushort(x)
    #define BSWAP32(x) _byteswap_ulong(x)
    #define BSWAP64(x) _byteswap_uint64(x)
#else
    #define BSWAP16(x) ((((x) & 0xFF00u) >> 8) | (((x) & 0x00FFu) << 8))
    #define BSWAP32(x) ((((x) & 0xFF000000u) >> 24) | (((x) & 0x00FF0000u) >> 8) | \
                        (((x) & 0x0000FF00u) << 8) | (((x) & 0x000000FFu) << 24))
    #define BSWAP64(x) (((uint64_t)BSWAP32((uint32_t)(x)) << 32) | BSWAP32((uint32_t)((x) >> 32)))
#endif

#if ENDIAN == 0
    #define LE16(x) (x)
    #define LE32(x) (x)
    #define LE64(x) (x)
    #define BE16(x) BSWAP16(x)
    #define BE32(x) BSWAP32(x)
    #define BE64(x) BSWAP64(x)
#elif ENDIAN == 1
    #define LE16(x) BSWAP16(x)
    #define LE32(x) BSWAP32(x)
    #define LE64(x) BSWAP64(x)
    #define BE16(x) (x)
    #define BE32(x) (x)
    #define BE64(x) (x)
#else
    #define LE16(x) (x)
    #define LE32(x) (x)
    #define LE64(x) (x)
    #define BE16(x) (x)
    #define BE32(x) (x)
    #define BE64(x) (x)
#endif

//      Operative System
/*
     0: Linux
     1: Android
     2: MacOS
     3: iOS
     4: FreeBSD
     5: Windows
     6: OpenBSD
     7: NetBSD
     8: DragonFly BSD
     9: QNX
    10: Zephyr RTOS
    11: FreeRTOS
    12: VxWorks
    13: NuttX
    14: RTEMS
    15: Haiku
    16: Fuchsia
    17: Emscripten (Web)
    18: Bare metal / Unknown
*/
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLATFORM 5
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_NAME "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define PLATFORM 3
        #define PLATFORM_IOS 1
        #define PLATFORM_NAME "iOS"
    #elif TARGET_OS_MAC
        #define PLATFORM 2
        #define PLATFORM_MACOS 1
        #define PLATFORM_NAME "macOS"
    #else
        #define PLATFORM 2
        #define PLATFORM_MACOS 1
        #define PLATFORM_NAME "Apple"
    #endif
    #define PLATFORM_APPLE 1
#elif defined(__ANDROID__)
    #define PLATFORM 1
    #define PLATFORM_ANDROID 1
    #define PLATFORM_NAME "Android"
#elif defined(__linux__) || defined(__linux) || defined(linux)
    #define PLATFORM 0
    #define PLATFORM_LINUX 1
    #define PLATFORM_NAME "Linux"
#elif defined(__FreeBSD__)
    #define PLATFORM 4
    #define PLATFORM_FREEBSD 1
    #define PLATFORM_NAME "FreeBSD"
#elif defined(__OpenBSD__)
    #define PLATFORM 6
    #define PLATFORM_OPENBSD 1
    #define PLATFORM_NAME "OpenBSD"
#elif defined(__NetBSD__)
    #define PLATFORM 7
    #define PLATFORM_NETBSD 1
    #define PLATFORM_NAME "NetBSD"
#elif defined(__DragonFly__)
    #define PLATFORM 8
    #define PLATFORM_DRAGONFLY 1
    #define PLATFORM_NAME "DragonFly"
#elif defined(__QNX__) || defined(__QNXNTO__)
    #define PLATFORM 9
    #define PLATFORM_QNX 1
    #define PLATFORM_NAME "QNX"
    #define PLATFORM_RTOS 1
#elif defined(__ZEPHYR__)
    #define PLATFORM 10
    #define PLATFORM_ZEPHYR 1
    #define PLATFORM_NAME "Zephyr"
    #define PLATFORM_RTOS 1
    #define PLATFORM_EMBEDDED 1
#elif defined(FREERTOS) || defined(INC_FREERTOS_H)
    #define PLATFORM 11
    #define PLATFORM_FREERTOS 1
    #define PLATFORM_NAME "FreeRTOS"
    #define PLATFORM_RTOS 1
    #define PLATFORM_EMBEDDED 1
#elif defined(__VXWORKS__) || defined(__vxworks)
    #define PLATFORM 12
    #define PLATFORM_VXWORKS 1
    #define PLATFORM_NAME "VxWorks"
    #define PLATFORM_RTOS 1
#elif defined(__nuttx__) || defined(CONFIG_ARCH_BOARD)
    #define PLATFORM 13
    #define PLATFORM_NUTTX 1
    #define PLATFORM_NAME "NuttX"
    #define PLATFORM_RTOS 1
    #define PLATFORM_EMBEDDED 1
#elif defined(__rtems__)
    #define PLATFORM 14
    #define PLATFORM_RTEMS 1
    #define PLATFORM_NAME "RTEMS"
    #define PLATFORM_RTOS 1
#elif defined(__HAIKU__)
    #define PLATFORM 15
    #define PLATFORM_HAIKU 1
    #define PLATFORM_NAME "Haiku"
#elif defined(__Fuchsia__)
    #define PLATFORM 16
    #define PLATFORM_FUCHSIA 1
    #define PLATFORM_NAME "Fuchsia"
#elif defined(__EMSCRIPTEN__)
    #define PLATFORM 17
    #define PLATFORM_EMSCRIPTEN 1
    #define PLATFORM_NAME "Emscripten"
    #define PLATFORM_WEB 1
#else
    #define PLATFORM 18
    #define PLATFORM_BAREMETAL 1
    #define PLATFORM_NAME "Bare Metal"
    #define PLATFORM_EMBEDDED 1
#endif

// POSIX compatibility
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_MACOS) || \
    defined(PLATFORM_IOS) || defined(PLATFORM_FREEBSD) || defined(PLATFORM_OPENBSD) || \
    defined(PLATFORM_NETBSD) || defined(PLATFORM_DRAGONFLY) || defined(PLATFORM_QNX) || \
    defined(PLATFORM_HAIKU)
    #define PLATFORM_POSIX 1
    #define POSIX 1
#else
    #define POSIX 0
#endif

// Threading support
#if defined(PLATFORM_RTOS) || defined(PLATFORM_POSIX) || defined(PLATFORM_WINDOWS)
    #define HAS_THREADS 1
#else
    #define HAS_THREADS 0
#endif

//      SIMD
#if defined(__AVX__)
    #ifndef __SSE2__
        #define __SSE2__
    #endif
    #ifndef __SSE3__
        #define __SSE3__
    #endif
    #ifndef __SSE4_1__
        #define __SSE4_1__
    #endif
#elif defined(_M_X64) || defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #ifndef __SSE2__
        #define __SSE2__
    #endif
#endif

#if defined(__ARM_NEON) && !defined(__ARM_NEON__)
    #define __ARM_NEON__
#endif

// SIMD feature flags
#ifdef __SSE__
    #define SIMD_SSE 1
#endif
#ifdef __SSE2__
    #define SIMD_SSE2 1
#endif
#ifdef __SSE3__
    #define SIMD_SSE3 1
#endif
#ifdef __SSSE3__
    #define SIMD_SSSE3 1
#endif
#ifdef __SSE4_1__
    #define SIMD_SSE41 1
#endif
#ifdef __SSE4_2__
    #define SIMD_SSE42 1
#endif
#ifdef __AVX__
    #define SIMD_AVX 1
#endif
#ifdef __AVX2__
    #define SIMD_AVX2 1
#endif
#ifdef __FMA__
    #define SIMD_FMA 1
#endif
#ifdef __AVX512F__
    #define SIMD_AVX512F 1
#endif
#ifdef __AVX512DQ__
    #define SIMD_AVX512DQ 1
#endif
#ifdef __AVX512BW__
    #define SIMD_AVX512BW 1
#endif
#ifdef __AVX512VL__
    #define SIMD_AVX512VL 1
#endif
#ifdef __ARM_NEON__
    #define SIMD_NEON 1
#endif
#if defined(__ARM_FEATURE_SVE)
    #define SIMD_SVE 1
#endif
#if defined(__ARM_FEATURE_SVE2)
    #define SIMD_SVE2 1
#endif
#if defined(__riscv_vector) || defined(__riscv_v)
    #define SIMD_RVV 1
#endif
#if defined(__ALTIVEC__) || defined(__VEC__)
    #define SIMD_ALTIVEC 1
#endif
#if defined(__VSX__)
    #define SIMD_VSX 1
#endif
#if defined(__mips_msa)
    #define SIMD_MSA 1
#endif
#if defined(__loongarch_sx)
    #define SIMD_LSX 1
#endif
#if defined(__loongarch_asx)
    #define SIMD_LASX 1
#endif
#if defined(__wasm_simd128__)
    #define SIMD_WASM128 1
#endif

// SIMD includes
#ifdef SIMD_SSE2
    #include <xmmintrin.h> // SSE
    #include <emmintrin.h> // SSE2
    #include <mmintrin.h>  // MMX
#endif
#ifdef SIMD_SSE3
    #include <pmmintrin.h> // SSE3
    #include <tmmintrin.h> // SSSE3
#endif
#ifdef SIMD_SSE41
    #include <smmintrin.h> // SSE4.1
#endif
#ifdef SIMD_SSE42
    #include <nmmintrin.h> // SSE4.2
#endif
#if defined(SIMD_AVX) || defined(SIMD_AVX2) || defined(SIMD_AVX512F)
    #include <immintrin.h> // AVX AVX2 AVX512
#endif
#ifdef SIMD_NEON
    #include <arm_neon.h>
#endif
#if defined(SIMD_SVE) || defined(SIMD_SVE2)
    #include <arm_sve.h>
#endif
#if defined(SIMD_RVV) && defined(__riscv_vector)
    #include <riscv_vector.h>
#endif
#if defined(SIMD_ALTIVEC) || defined(SIMD_VSX)
    #include <altivec.h>
#endif
#ifdef SIMD_MSA
    #include <msa.h>
#endif
#if defined(SIMD_LSX) || defined(SIMD_LASX)
    #include <lsxintrin.h>
    #ifdef SIMD_LASX
        #include <lasxintrin.h>
    #endif
#endif
#ifdef SIMD_WASM128
    #include <wasm_simd128.h>
#endif

// SIMD alignment
#if defined(SIMD_AVX512F)
    #define SIMD_ALIGNMENT 64
    #define SIMD_WIDTH_BYTES 64
    #define SIMD_WIDTH_FLOAT 16
    #define SIMD_WIDTH_INT32 16
#elif defined(SIMD_AVX) || defined(SIMD_AVX2) || defined(SIMD_LASX)
    #define SIMD_ALIGNMENT 32
    #define SIMD_WIDTH_BYTES 32
    #define SIMD_WIDTH_FLOAT 8
    #define SIMD_WIDTH_INT32 8
#elif defined(SIMD_SVE) || defined(SIMD_SVE2)
    #define SIMD_ALIGNMENT 256
    #define SIMD_WIDTH_BYTES 256
    #define SIMD_WIDTH_FLOAT 64
    #define SIMD_WIDTH_INT32 64
#elif defined(SIMD_SSE2) || defined(SIMD_NEON) || defined(SIMD_VSX) || \
      defined(SIMD_MSA) || defined(SIMD_WASM128) || defined(SIMD_LSX) || \
      defined(SIMD_ALTIVEC)
    #define SIMD_ALIGNMENT 16
    #define SIMD_WIDTH_BYTES 16
    #define SIMD_WIDTH_FLOAT 4
    #define SIMD_WIDTH_INT32 4
#else
    #define SIMD_ALIGNMENT 4
    #define SIMD_WIDTH_BYTES 4
    #define SIMD_WIDTH_FLOAT 1
    #define SIMD_WIDTH_INT32 1
    #define SIMD_NONE 1
#endif

// Backward compatibility
#define SIMD_ALIGNMENT_FLOAT SIMD_ALIGNMENT
#define SIMD_ALIGNMENT_INT SIMD_ALIGNMENT

// CPU pause hint for spin loops
#if defined(SIMD_SSE2)
    #define cpu_pause() _mm_pause()
#elif defined(ARCH_ARM64) || defined(ARCH_ARM32)
    #if defined(COMPILER_GNU_COMPAT)
        #define cpu_pause() __asm__ __volatile__("yield" ::: "memory")
    #else
        #define cpu_pause() ((void)0)
    #endif
#elif defined(ARCH_PPC32) || defined(ARCH_PPC64)
    #if defined(COMPILER_GNU_COMPAT)
        #define cpu_pause() __asm__ __volatile__("or 27,27,27" ::: "memory")
    #else
        #define cpu_pause() ((void)0)
    #endif
#elif defined(ARCH_RISCV32) || defined(ARCH_RISCV64)
    #if defined(COMPILER_GNU_COMPAT)
        #define cpu_pause() __asm__ __volatile__(".insn i 0x0F, 0, x0, x0, 0x010" ::: "memory")
    #else
        #define cpu_pause() ((void)0)
    #endif
#else
    #define cpu_pause() ((void)0)
#endif

//      Compiler
/*
    0: GCC
    1: MSVC
    2: Clang
    3: Intel ICC/ICX
    4: ARM Compiler
    5: TinyCC
    6: PGI/NVIDIA HPC
    7: IBM XL C
    8: Sun/Oracle Studio
    9: Unknown
*/
#if defined(__INTEL_COMPILER) || defined(__ICC) || defined(__INTEL_LLVM_COMPILER)
    #define COMPILER 3
    #define COMPILER_ICC 1
    #define COMPILER_NAME "Intel"
    #define COMPILER_VERSION __INTEL_COMPILER
#elif defined(__clang__)
    #define COMPILER 2
    #define COMPILER_CLANG 1
    #define COMPILER_NAME "Clang"
    #define COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
    #define COMPILER 1
    #define COMPILER_MSVC 1
    #define COMPILER_NAME "MSVC"
    #define COMPILER_VERSION _MSC_VER
#elif defined(__GNUC__)
    #define COMPILER 0
    #define COMPILER_GCC 1
    #define COMPILER_NAME "GCC"
    #define COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__ARMCC_VERSION) || defined(__CC_ARM)
    #define COMPILER 4
    #define COMPILER_ARMCC 1
    #define COMPILER_NAME "ARM CC"
    #define COMPILER_VERSION __ARMCC_VERSION
#elif defined(__TINYC__)
    #define COMPILER 5
    #define COMPILER_TCC 1
    #define COMPILER_NAME "TinyCC"
    #define COMPILER_VERSION 0
#elif defined(__PGI) || defined(__NVCOMPILER)
    #define COMPILER 6
    #define COMPILER_PGI 1
    #define COMPILER_NAME "PGI/NVIDIA"
    #ifdef __PGIC__
        #define COMPILER_VERSION (__PGIC__ * 10000)
    #else
        #define COMPILER_VERSION 0
    #endif
#elif defined(__xlC__) || defined(__ibmxl__)
    #define COMPILER 7
    #define COMPILER_XLC 1
    #define COMPILER_NAME "IBM XL C"
    #define COMPILER_VERSION __xlC__
#elif defined(__SUNPRO_C)
    #define COMPILER 8
    #define COMPILER_SUNPRO 1
    #define COMPILER_NAME "Sun Studio"
    #define COMPILER_VERSION __SUNPRO_C
#else
    #define COMPILER 9
    #define COMPILER_UNKNOWN 1
    #define COMPILER_NAME "Unknown"
    #define COMPILER_VERSION 0
#endif

// GNU-compatible compilers (GCC, Clang, ICC, etc.)
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    #define COMPILER_GNU_COMPAT 1
#endif

//      Compiler Utility
#if defined(COMPILER_MSVC)
    #define force_inline __forceinline
    #define no_inline __declspec(noinline)
    #if defined(ARCH_X64) || defined(ARCH_X86)
        #include <intrin.h>
        #define prefetch_read(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
        #define prefetch_write(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
    #else
        #define prefetch_read(addr) ((void)(addr))
        #define prefetch_write(addr) ((void)(addr))
    #endif
    #define expect(x, expectation) (x)
    #define UNLIKELY(x) (x)
    #define LIKELY(x) (x)
    #define THREAD_LOCAL __declspec(thread)
    #define RESTRICT __restrict
#elif defined(COMPILER_GNU_COMPAT)
    #define force_inline __attribute__((always_inline)) inline
    #define no_inline __attribute__((noinline))
    #define prefetch_read(addr) __builtin_prefetch((const void*)(addr), 0, 3)
    #define prefetch_write(addr) __builtin_prefetch((const void*)(addr), 1, 3)
    #define expect(x, expectation) __builtin_expect((long)(x), (long)(expectation))
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define THREAD_LOCAL __thread
    #define RESTRICT __restrict__
#elif defined(COMPILER_ARMCC)
    #define force_inline __forceinline
    #define no_inline __attribute__((noinline))
    #define prefetch_read(addr) __pld((const void*)(addr))
    #define prefetch_write(addr) __pld((const void*)(addr))
    #define expect(x, expectation) (x)
    #define UNLIKELY(x) (x)
    #define LIKELY(x) (x)
    #define THREAD_LOCAL __thread
    #define RESTRICT __restrict
#else
    #define force_inline inline
    #define no_inline
    #define prefetch_read(addr) ((void)(addr))
    #define prefetch_write(addr) ((void)(addr))
    #define expect(x, expectation) (x)
    #define UNLIKELY(x) (x)
    #define LIKELY(x) (x)
    #define RESTRICT
    // No thread local support
    #define THREAD_LOCAL
    #define NO_THREAD_LOCAL 1
#endif

// Function attributes
#if defined(COMPILER_MSVC)
    #define NORETURN_ATTRIBUTE __declspec(noreturn)
    #define UNUSED_ATTRIBUTE
    #define PURE_ATTRIBUTE
    #define DEPRECATED_ATTRIBUTE __declspec(deprecated)
    #define EXPORT_ATTRIBUTE __declspec(dllexport)
    #define IMPORT_ATTRIBUTE __declspec(dllimport)
    #define PACKED_ATTRIBUTE
    #define FORMAT_ATTRIBUTE(fmt, first)
#elif defined(COMPILER_GNU_COMPAT)
    #define NORETURN_ATTRIBUTE __attribute__((noreturn))
    #define UNUSED_ATTRIBUTE __attribute__((unused))
    #define PURE_ATTRIBUTE __attribute__((pure))
    #define DEPRECATED_ATTRIBUTE __attribute__((deprecated))
    #define EXPORT_ATTRIBUTE __attribute__((visibility("default")))
    #define IMPORT_ATTRIBUTE
    #define PACKED_ATTRIBUTE __attribute__((packed))
    #define FORMAT_ATTRIBUTE(fmt, first) __attribute__((format(printf, fmt, first)))
#else
    #define NORETURN_ATTRIBUTE
    #define UNUSED_ATTRIBUTE
    #define PURE_ATTRIBUTE
    #define DEPRECATED_ATTRIBUTE
    #define EXPORT_ATTRIBUTE
    #define IMPORT_ATTRIBUTE
    #define PACKED_ATTRIBUTE
    #define FORMAT_ATTRIBUTE(fmt, first)
#endif

//      C Standard (C89+)
#if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 202311L
        #define C_STD 23
        #define C23_SUPPORTED 1
    #elif __STDC_VERSION__ >= 201710L
        #define C_STD 17
    #elif __STDC_VERSION__ >= 201112L
        #define C_STD 11
    #elif __STDC_VERSION__ >= 199901L
        #define C_STD 99
    #else
        #define C_STD 89
    #endif
#else
    #define C_STD 89
#endif

// Inline keyword for C89
#if C_STD < 99 && !defined(COMPILER_MSVC)
    #if defined(COMPILER_GNU_COMPAT)
        #define inline __inline__
    #else
        #define inline
    #endif
#endif

// alignas / alignof
#if C_STD >= 11 && !defined(COMPILER_MSVC)
    #include <stdalign.h>
#elif defined(COMPILER_MSVC)
    #define alignas(n) __declspec(align(n))
    #define alignof(t) __alignof(t)
#elif defined(COMPILER_GNU_COMPAT)
    #define alignas(n) __attribute__((aligned(n)))
    #define alignof(t) __alignof__(t)
#else
    #define alignas(n)
    #define alignof(t) sizeof(t)
#endif

// unreachable - guard against redefinition (C23 stddef.h may define it)
#ifndef unreachable
    #if defined(COMPILER_MSVC)
        #define unreachable() __assume(0)
    #elif defined(COMPILER_GNU_COMPAT)
        #define unreachable() __builtin_unreachable()
    #else
        static force_inline void unreachable_impl(void) { for(;;); }
        #define unreachable() unreachable_impl()
    #endif
#endif

// Aligned memory allocation
#if defined(COMPILER_MSVC)
    #include <malloc.h>
    #define aligned_alloc(alignment, size) _aligned_malloc((size), (alignment))
    #define aligned_free(ptr) _aligned_free(ptr)
#elif C_STD >= 11 && !defined(PLATFORM_ANDROID)
    #include <stdlib.h>
    #define aligned_free(ptr) free(ptr)
#elif POSIX
    #include <stdlib.h>
    static force_inline void* aligned_alloc_impl(size_t alignment, size_t size) {
        void* p = NULL;
        if (posix_memalign(&p, alignment, size) != 0) return NULL;
        return p;
    }
    #define aligned_alloc(alignment, size) aligned_alloc_impl((alignment), (size))
    #define aligned_free(ptr) free(ptr)
#else
    // Fallback: manual alignment
    #include <stdlib.h>
    static force_inline void* aligned_alloc(size_t alignment, size_t size) {
        void* p = malloc(size + alignment + sizeof(void*));
        if (!p) return NULL;
        void** aligned = (void**)(((size_t)p + alignment + sizeof(void*)) & ~(alignment - 1));
        aligned[-1] = p;
        return aligned;
    }
    static force_inline void aligned_free(void* ptr) {
        if (ptr) free(((void**)ptr)[-1]);
    }
#endif

//      Utility Macros

// Min/Max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(MAX(x, lo), hi))

// Array size
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Bit manipulation
#define BIT(n) (1UL << (n))
#define SET_BIT(x, n) ((x) |= BIT(n))
#define CLEAR_BIT(x, n) ((x) &= ~BIT(n))
#define TOGGLE_BIT(x, n) ((x) ^= BIT(n))
#define TEST_BIT(x, n) (((x) & BIT(n)) != 0)

// Alignment
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define IS_ALIGNED(x, a) (((x) & ((a) - 1)) == 0)

// Pointer arithmetic
#define PTR_ADD(p, n) ((void*)((char*)(p) + (n)))
#define PTR_SUB(p, n) ((void*)((char*)(p) - (n)))
#define PTR_DIFF(a, b) ((size_t)((char*)(a) - (char*)(b)))

// Stringify / Concatenate
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

// Unused parameter
#define UNUSED(x) ((void)(x))

// Assertions
#define ASSERT(x) \
    do { \
        if (UNLIKELY(!(x))) unreachable(); \
    } while(0)

#ifdef NDEBUG
    #define DEBUG_ASSERT(x) ((void)0)
#else
    #define DEBUG_ASSERT(x) ASSERT(x)
#endif

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG) || defined(COMPILER_TCC)
    #define COMPUTED_GOTO_SUPPORTED 1
#else
    #define COMPUTED_GOTO_SUPPORTED 0
#endif

// Build configuration
#if defined(NDEBUG) || defined(RELEASE)
    #define BUILD_RELEASE 1
    #define BUILD_TYPE "Release"
#else
    #define BUILD_DEBUG 1
    #define BUILD_TYPE "Debug"
#endif

#endif
