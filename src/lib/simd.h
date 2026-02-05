#ifndef SIMD_H_INCLUDED
#define SIMD_H_INCLUDED

/* ==========================================
 * SIMD Utilities Header
 * 
 * Provides:
 * - Unified SIMD header includes for all architectures
 * - SIMD detection macros and alignment constants
 * - Utility functions NOT available in standard library
 * 
 * Note: For memory operations (memcpy, memset, memcmp, memchr)
 * ALWAYS use the standard library. Modern libc implementations
 * use runtime CPU detection and are faster than compile-time
 * SIMD implementations.
 * 
 * Supported architectures:
 * - x86/x64: SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX-512
 * - ARM: NEON, SVE, SVE2
 * - RISC-V: RVV (Vector Extension)
 * - PowerPC: AltiVec, VSX
 * - MIPS: MSA
 * - LoongArch: LSX, LASX
 * - WebAssembly: SIMD128
 * 
 * Requires: macros.h
 * ========================================== */

#include "macros.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>  /* Use standard library for memory ops */

/* ==========================================
 * SIMD HEADERS
 * ========================================== */

/* x86/x64 intrinsics */
#if defined(ARCH_X86) || defined(ARCH_X64)
    #if SIMD_AVX512F || SIMD_AVX2 || SIMD_AVX
        #include <immintrin.h>
    #elif SIMD_SSE42
        #include <nmmintrin.h>
    #elif SIMD_SSE41
        #include <smmintrin.h>
    #elif SIMD_SSSE3
        #include <tmmintrin.h>
    #elif SIMD_SSE3
        #include <pmmintrin.h>
    #elif SIMD_SSE2
        #include <emmintrin.h>
    #elif SIMD_SSE
        #include <xmmintrin.h>
    #endif
#endif

/* ARM NEON */
#if SIMD_NEON
    #include <arm_neon.h>
#endif

/* ARM SVE/SVE2 */
#if SIMD_SVE || SIMD_SVE2
    #include <arm_sve.h>
#endif

/* PowerPC AltiVec/VSX */
#if SIMD_ALTIVEC || SIMD_VSX
    #include <altivec.h>
    /* Avoid AltiVec keyword conflicts with C++ */
    #ifdef bool
        #undef bool
    #endif
    #ifdef vector
        #undef vector
    #endif
    #ifdef pixel
        #undef pixel
    #endif
#endif

/* RISC-V Vector */
#if SIMD_RVV
    #include <riscv_vector.h>
#endif

/* MIPS MSA */
#if SIMD_MSA
    #include <msa.h>
#endif

/* LoongArch LSX/LASX */
#if SIMD_LSX
    #include <lsxintrin.h>
#endif
#if SIMD_LASX
    #include <lasxintrin.h>
#endif

/* WebAssembly SIMD */
#if SIMD_WASM128
    #include <wasm_simd128.h>
#endif

/* ==========================================
 * ALIGNMENT REQUIREMENTS
 * ========================================== */

#if SIMD_AVX512F
    #define SIMD_ALIGN 64
    #define SIMD_VECTOR_SIZE 64
#elif SIMD_AVX || SIMD_AVX2 || SIMD_LASX
    #define SIMD_ALIGN 32
    #define SIMD_VECTOR_SIZE 32
#elif SIMD_SVE || SIMD_SVE2
    /* SVE: scalable vectors, use 32 for common implementations */
    #define SIMD_ALIGN 32
    #define SIMD_VECTOR_SIZE 32
#elif SIMD_SSE2 || SIMD_NEON || SIMD_ALTIVEC || SIMD_VSX || \
      SIMD_MSA || SIMD_WASM128 || SIMD_LSX || SIMD_RVV
    #define SIMD_ALIGN 16
    #define SIMD_VECTOR_SIZE 16
#else
    #define SIMD_ALIGN 8
    #define SIMD_VECTOR_SIZE 8
    #define SIMD_NONE 1
#endif

/* ==========================================
 * SIMD LEVEL DETECTION
 * ========================================== */

/* Higher = more capable (approximate) */
#if SIMD_AVX512F
    #define SIMD_LEVEL 7
    #define SIMD_NAME "AVX-512"
#elif SIMD_SVE2
    #define SIMD_LEVEL 7
    #define SIMD_NAME "SVE2"
#elif SIMD_AVX2
    #define SIMD_LEVEL 6
    #define SIMD_NAME "AVX2"
#elif SIMD_SVE
    #define SIMD_LEVEL 6
    #define SIMD_NAME "SVE"
#elif SIMD_AVX || SIMD_LASX
    #define SIMD_LEVEL 5
    #define SIMD_NAME "AVX"
#elif SIMD_SSE42
    #define SIMD_LEVEL 4
    #define SIMD_NAME "SSE4.2"
#elif SIMD_SSE41 || SIMD_VSX || SIMD_RVV
    #define SIMD_LEVEL 3
    #define SIMD_NAME "SSE4.1"
#elif SIMD_SSE2 || SIMD_NEON || SIMD_ALTIVEC || SIMD_MSA || SIMD_LSX || SIMD_WASM128
    #define SIMD_LEVEL 2
    #define SIMD_NAME "SSE2/NEON"
#else
    #define SIMD_LEVEL 0
    #define SIMD_NAME "Scalar"
#endif

/* ==========================================
 * BYTE SUM (not in standard library)
 * 
 * Sum all bytes in a buffer - useful for checksums,
 * hash seeding, or simple integrity checks.
 * ========================================== */

static force_inline uint64_t simd_byte_sum(const void* ptr, size_t size) {
    const uint8_t* p = (const uint8_t*)ptr;
    uint64_t sum = 0;
    
#if SIMD_AVX512F
    __m512i acc = _mm512_setzero_si512();
    while (size >= 64) {
        __m512i data = _mm512_loadu_si512((const __m512i*)p);
        __m512i sad = _mm512_sad_epu8(data, _mm512_setzero_si512());
        acc = _mm512_add_epi64(acc, sad);
        p += 64;
        size -= 64;
    }
    sum = _mm512_reduce_add_epi64(acc);
#elif SIMD_AVX2
    __m256i acc = _mm256_setzero_si256();
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)p);
        __m256i sad = _mm256_sad_epu8(data, _mm256_setzero_si256());
        acc = _mm256_add_epi64(acc, sad);
        p += 32;
        size -= 32;
    }
    __m128i low = _mm256_castsi256_si128(acc);
    __m128i high = _mm256_extracti128_si256(acc, 1);
    __m128i sum128 = _mm_add_epi64(low, high);
    sum = (uint64_t)_mm_extract_epi64(sum128, 0) + (uint64_t)_mm_extract_epi64(sum128, 1);
#elif SIMD_SSE2
    __m128i acc = _mm_setzero_si128();
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        __m128i sad = _mm_sad_epu8(data, _mm_setzero_si128());
        acc = _mm_add_epi64(acc, sad);
        p += 16;
        size -= 16;
    }
    sum = (uint64_t)_mm_cvtsi128_si64(acc) + 
          (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(acc, 8));
#elif SIMD_NEON
    uint64x2_t acc = vdupq_n_u64(0);
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(p);
        uint16x8_t sum16 = vpaddlq_u8(data);
        uint32x4_t sum32 = vpaddlq_u16(sum16);
        acc = vaddq_u64(acc, vpaddlq_u32(sum32));
        p += 16;
        size -= 16;
    }
    sum = vgetq_lane_u64(acc, 0) + vgetq_lane_u64(acc, 1);
#elif SIMD_SVE || SIMD_SVE2
    uint64_t sve_sum = 0;
    while (size >= svcntb()) {
        svbool_t pg = svptrue_b8();
        svuint8_t data = svld1_u8(pg, p);
        sve_sum += svaddv_u8(pg, data);
        p += svcntb();
        size -= svcntb();
    }
    if (size > 0) {
        svbool_t pg = svwhilelt_b8((uint64_t)0, size);
        svuint8_t data = svld1_u8(pg, p);
        sve_sum += svaddv_u8(pg, data);
        return sve_sum;
    }
    sum = sve_sum;
#elif SIMD_VSX
    __vector unsigned long long acc = {0, 0};
    while (size >= 16) {
        __vector unsigned char data = vec_vsx_ld(0, p);
        /* Sum pairs progressively */
        __vector unsigned short sum16 = (__vector unsigned short)vec_sum4s(data, (__vector unsigned char){0});
        __vector unsigned int sum32 = vec_sum4s((__vector signed short)sum16, (__vector signed int){0});
        acc = vec_add(acc, (__vector unsigned long long)sum32);
        p += 16;
        size -= 16;
    }
    sum = acc[0] + acc[1];
#elif SIMD_RVV
    while (size > 0) {
        size_t vl = __riscv_vsetvl_e8m8(size);
        vuint8m8_t data = __riscv_vle8_v_u8m8(p, vl);
        /* Reduce with widening */
        vuint16m1_t vsum16 = __riscv_vmv_s_x_u16m1(0, 1);
        vsum16 = __riscv_vwredsumu_vs_u8m8_u16m1(data, vsum16, vl);
        sum += __riscv_vmv_x_s_u16m1_u16(vsum16);
        p += vl;
        size -= vl;
    }
    return sum;
#endif
    
    /* Scalar cleanup */
    while (size > 0) {
        sum += *p++;
        size--;
    }
    return sum;
}

/* ==========================================
 * POPULATION COUNT (not always in stdlib)
 * 
 * Count set bits in a buffer.
 * ========================================== */

static force_inline uint64_t simd_popcount(const void* ptr, size_t size) {
    const uint8_t* p = (const uint8_t*)ptr;
    uint64_t count = 0;
    
#if SIMD_AVX512F && defined(__AVX512VPOPCNTDQ__)
    while (size >= 64) {
        __m512i data = _mm512_loadu_si512((const __m512i*)p);
        __m512i popcnt = _mm512_popcnt_epi64(data);
        count += _mm512_reduce_add_epi64(popcnt);
        p += 64;
        size -= 64;
    }
#elif SIMD_AVX2
    /* Use lookup table method for popcount */
    const __m256i lookup = _mm256_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    __m256i acc = _mm256_setzero_si256();
    
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)p);
        __m256i lo = _mm256_and_si256(data, low_mask);
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(data, 4), low_mask);
        __m256i popcnt = _mm256_add_epi8(_mm256_shuffle_epi8(lookup, lo),
                                         _mm256_shuffle_epi8(lookup, hi));
        acc = _mm256_add_epi64(acc, _mm256_sad_epu8(popcnt, _mm256_setzero_si256()));
        p += 32;
        size -= 32;
    }
    __m128i low = _mm256_castsi256_si128(acc);
    __m128i high = _mm256_extracti128_si256(acc, 1);
    __m128i sum128 = _mm_add_epi64(low, high);
    count = (uint64_t)_mm_extract_epi64(sum128, 0) + (uint64_t)_mm_extract_epi64(sum128, 1);
#elif SIMD_SSE2 && SIMD_SSSE3
    const __m128i lookup = _mm_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m128i low_mask = _mm_set1_epi8(0x0F);
    __m128i acc = _mm_setzero_si128();
    
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        __m128i lo = _mm_and_si128(data, low_mask);
        __m128i hi = _mm_and_si128(_mm_srli_epi16(data, 4), low_mask);
        __m128i popcnt = _mm_add_epi8(_mm_shuffle_epi8(lookup, lo),
                                      _mm_shuffle_epi8(lookup, hi));
        acc = _mm_add_epi64(acc, _mm_sad_epu8(popcnt, _mm_setzero_si128()));
        p += 16;
        size -= 16;
    }
    count = (uint64_t)_mm_cvtsi128_si64(acc) + 
            (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(acc, 8));
#elif SIMD_NEON
    uint64x2_t acc = vdupq_n_u64(0);
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(p);
        uint8x16_t popcnt = vcntq_u8(data);
        uint16x8_t sum16 = vpaddlq_u8(popcnt);
        uint32x4_t sum32 = vpaddlq_u16(sum16);
        acc = vaddq_u64(acc, vpaddlq_u32(sum32));
        p += 16;
        size -= 16;
    }
    count = vgetq_lane_u64(acc, 0) + vgetq_lane_u64(acc, 1);
#endif

    /* Scalar cleanup using compiler builtin if available */
    while (size >= 8) {
        uint64_t val;
        memcpy(&val, p, 8);
#if defined(COMPILER_GNU_COMPAT)
        count += (uint64_t)__builtin_popcountll(val);
#elif defined(COMPILER_MSVC) && defined(ARCH_X64)
        count += (uint64_t)__popcnt64(val);
#else
        /* Fallback popcount */
        val = val - ((val >> 1) & 0x5555555555555555ULL);
        val = (val & 0x3333333333333333ULL) + ((val >> 2) & 0x3333333333333333ULL);
        count += (((val + (val >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56;
#endif
        p += 8;
        size -= 8;
    }
    
    /* Remaining bytes */
    while (size > 0) {
        uint8_t val = *p++;
#if defined(COMPILER_GNU_COMPAT)
        count += (uint64_t)__builtin_popcount(val);
#else
        val = val - ((val >> 1) & 0x55);
        val = (val & 0x33) + ((val >> 2) & 0x33);
        count += (val + (val >> 4)) & 0x0F;
#endif
        size--;
    }
    return count;
}

/* ==========================================
 * HORIZONTAL OPERATIONS
 * 
 * These operations are useful for reductions
 * and are not trivially available in stdlib.
 * ========================================== */

/* Find minimum byte value in buffer */
static force_inline uint8_t simd_min_u8(const void* ptr, size_t size) {
    if (size == 0) return 0xFF;
    const uint8_t* p = (const uint8_t*)ptr;
    uint8_t min_val = 0xFF;
    
#if SIMD_AVX512BW
    __m512i vmin = _mm512_set1_epi8((char)0xFF);
    while (size >= 64) {
        __m512i data = _mm512_loadu_si512((const __m512i*)p);
        vmin = _mm512_min_epu8(vmin, data);
        p += 64;
        size -= 64;
    }
    /* Reduce 512-bit to 256-bit then to scalar */
    __m256i low256 = _mm512_castsi512_si256(vmin);
    __m256i high256 = _mm512_extracti64x4_epi64(vmin, 1);
    __m256i min256 = _mm256_min_epu8(low256, high256);
    __m128i low = _mm256_castsi256_si128(min256);
    __m128i high = _mm256_extracti128_si256(min256, 1);
    __m128i min128 = _mm_min_epu8(low, high);
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 8));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 4));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 2));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 1));
    min_val = (uint8_t)_mm_extract_epi8(min128, 0);
#elif SIMD_AVX2
    __m256i vmin = _mm256_set1_epi8((char)0xFF);
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)p);
        vmin = _mm256_min_epu8(vmin, data);
        p += 32;
        size -= 32;
    }
    /* Reduce 256-bit to scalar */
    __m128i low = _mm256_castsi256_si128(vmin);
    __m128i high = _mm256_extracti128_si256(vmin, 1);
    __m128i min128 = _mm_min_epu8(low, high);
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 8));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 4));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 2));
    min128 = _mm_min_epu8(min128, _mm_srli_si128(min128, 1));
    min_val = (uint8_t)_mm_extract_epi8(min128, 0);
#elif SIMD_SSE2
    __m128i vmin = _mm_set1_epi8((char)0xFF);
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        vmin = _mm_min_epu8(vmin, data);
        p += 16;
        size -= 16;
    }
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 8));
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 4));
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 2));
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 1));
    min_val = (uint8_t)(_mm_extract_epi16(vmin, 0) & 0xFF);
#elif SIMD_SVE || SIMD_SVE2
    uint8_t sve_min = 0xFF;
    while (size >= svcntb()) {
        svbool_t pg = svptrue_b8();
        svuint8_t data = svld1_u8(pg, p);
        sve_min = MIN(sve_min, svminv_u8(pg, data));
        p += svcntb();
        size -= svcntb();
    }
    if (size > 0) {
        svbool_t pg = svwhilelt_b8((uint64_t)0, size);
        svuint8_t data = svld1_u8(pg, p);
        sve_min = MIN(sve_min, svminv_u8(pg, data));
        return sve_min;
    }
    min_val = sve_min;
#elif SIMD_NEON
    uint8x16_t vmin = vdupq_n_u8(0xFF);
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(p);
        vmin = vminq_u8(vmin, data);
        p += 16;
        size -= 16;
    }
    min_val = vminvq_u8(vmin);
#endif
    
    /* Scalar cleanup */
    while (size > 0) {
        if (*p < min_val) min_val = *p;
        p++;
        size--;
    }
    return min_val;
}

/* Find maximum byte value in buffer */
static force_inline uint8_t simd_max_u8(const void* ptr, size_t size) {
    if (size == 0) return 0;
    const uint8_t* p = (const uint8_t*)ptr;
    uint8_t max_val = 0;
    
#if SIMD_AVX512BW
    __m512i vmax = _mm512_setzero_si512();
    while (size >= 64) {
        __m512i data = _mm512_loadu_si512((const __m512i*)p);
        vmax = _mm512_max_epu8(vmax, data);
        p += 64;
        size -= 64;
    }
    /* Reduce 512-bit to scalar */
    __m256i low256 = _mm512_castsi512_si256(vmax);
    __m256i high256 = _mm512_extracti64x4_epi64(vmax, 1);
    __m256i max256 = _mm256_max_epu8(low256, high256);
    __m128i low = _mm256_castsi256_si128(max256);
    __m128i high = _mm256_extracti128_si256(max256, 1);
    __m128i max128 = _mm_max_epu8(low, high);
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 8));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 4));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 2));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 1));
    max_val = (uint8_t)_mm_extract_epi8(max128, 0);
#elif SIMD_AVX2
    __m256i vmax = _mm256_setzero_si256();
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)p);
        vmax = _mm256_max_epu8(vmax, data);
        p += 32;
        size -= 32;
    }
    __m128i low = _mm256_castsi256_si128(vmax);
    __m128i high = _mm256_extracti128_si256(vmax, 1);
    __m128i max128 = _mm_max_epu8(low, high);
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 8));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 4));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 2));
    max128 = _mm_max_epu8(max128, _mm_srli_si128(max128, 1));
    max_val = (uint8_t)_mm_extract_epi8(max128, 0);
#elif SIMD_SSE2
    __m128i vmax = _mm_setzero_si128();
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        vmax = _mm_max_epu8(vmax, data);
        p += 16;
        size -= 16;
    }
    vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 8));
    vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 4));
    vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 2));
    vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 1));
    max_val = (uint8_t)(_mm_extract_epi16(vmax, 0) & 0xFF);
#elif SIMD_SVE || SIMD_SVE2
    uint8_t sve_max = 0;
    while (size >= svcntb()) {
        svbool_t pg = svptrue_b8();
        svuint8_t data = svld1_u8(pg, p);
        sve_max = MAX(sve_max, svmaxv_u8(pg, data));
        p += svcntb();
        size -= svcntb();
    }
    if (size > 0) {
        svbool_t pg = svwhilelt_b8((uint64_t)0, size);
        svuint8_t data = svld1_u8(pg, p);
        sve_max = MAX(sve_max, svmaxv_u8(pg, data));
        return sve_max;
    }
    max_val = sve_max;
#elif SIMD_NEON
    uint8x16_t vmax = vdupq_n_u8(0);
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(p);
        vmax = vmaxq_u8(vmax, data);
        p += 16;
        size -= 16;
    }
    max_val = vmaxvq_u8(vmax);
#endif
    
    /* Scalar cleanup */
    while (size > 0) {
        if (*p > max_val) max_val = *p;
        p++;
        size--;
    }
    return max_val;
}

/* ==========================================
 * CHECK IF ALL BYTES ARE ZERO
 * ========================================== */

static force_inline int simd_is_zero(const void* ptr, size_t size) {
    const uint8_t* p = (const uint8_t*)ptr;
    
#if SIMD_AVX512F
    while (size >= 64) {
        __m512i data = _mm512_loadu_si512((const __m512i*)p);
        if (_mm512_test_epi64_mask(data, data) != 0) return 0;
        p += 64;
        size -= 64;
    }
#endif
#if SIMD_AVX2
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)p);
        if (!_mm256_testz_si256(data, data)) return 0;
        p += 32;
        size -= 32;
    }
#elif SIMD_SSE41
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        if (!_mm_testz_si128(data, data)) return 0;
        p += 16;
        size -= 16;
    }
#elif SIMD_SSE2
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)p);
        __m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());
        if (_mm_movemask_epi8(cmp) != 0xFFFF) return 0;
        p += 16;
        size -= 16;
    }
#elif SIMD_SVE || SIMD_SVE2
    while (size >= svcntb()) {
        svbool_t pg = svptrue_b8();
        svuint8_t data = svld1_u8(pg, p);
        if (svptest_any(pg, svcmpne_n_u8(pg, data, 0))) return 0;
        p += svcntb();
        size -= svcntb();
    }
    if (size > 0) {
        svbool_t pg = svwhilelt_b8((uint64_t)0, size);
        svuint8_t data = svld1_u8(pg, p);
        if (svptest_any(pg, svcmpne_n_u8(pg, data, 0))) return 0;
        return 1;
    }
#elif SIMD_NEON
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(p);
        uint64x2_t cmp = vreinterpretq_u64_u8(data);
        if (vgetq_lane_u64(cmp, 0) != 0 || vgetq_lane_u64(cmp, 1) != 0) return 0;
        p += 16;
        size -= 16;
    }
#endif
    
    /* Scalar cleanup */
    while (size > 0) {
        if (*p != 0) return 0;
        p++;
        size--;
    }
    return 1;
}

#endif /* SIMD_H_INCLUDED */
