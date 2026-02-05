#include "cpu.h"
#include <string.h>
#include <stdio.h>

/* ==========================================
 * CPU Detection Implementation
 * ========================================== */

/* Cached CPU info (initialized once) */
static struct cpu_info g_cpu_info = {0};
static int g_cpu_info_init = 0;

/* ==========================================
 * x86/x64 CPUID
 * ========================================== */

#if defined(ARCH_X86) || defined(ARCH_X64)

#if defined(COMPILER_MSVC)
    #include <intrin.h>
    #define CPUID(regs, leaf) __cpuidex(regs, leaf, 0)
    #define CPUID_EX(regs, leaf, sub) __cpuidex(regs, leaf, sub)
#else
    #include <cpuid.h>
    #define CPUID(regs, leaf) __cpuid_count(leaf, 0, regs[0], regs[1], regs[2], regs[3])
    #define CPUID_EX(regs, leaf, sub) __cpuid_count(leaf, sub, regs[0], regs[1], regs[2], regs[3])
#endif

static void detect_x86_features(struct cpu_info* info) {
    int regs[4] = {0};
    
    /* Get vendor string */
    CPUID(regs, 0);
    int max_leaf = regs[0];
    memcpy(info->vendor, &regs[1], 4);
    memcpy(info->vendor + 4, &regs[3], 4);
    memcpy(info->vendor + 8, &regs[2], 4);
    info->vendor[12] = '\0';
    
    /* Get brand string */
    CPUID(regs, 0x80000000);
    if ((unsigned)regs[0] >= 0x80000004) {
        CPUID(regs, 0x80000002);
        memcpy(info->brand, regs, 16);
        CPUID(regs, 0x80000003);
        memcpy(info->brand + 16, regs, 16);
        CPUID(regs, 0x80000004);
        memcpy(info->brand + 32, regs, 16);
        info->brand[48] = '\0';
    }
    
    /* Basic feature flags (leaf 1) */
    if (max_leaf >= 1) {
        CPUID(regs, 1);
        info->has_sse2  = (regs[3] & (1 << 26)) != 0;
        info->has_sse3  = (regs[2] & (1 << 0)) != 0;
        info->has_ssse3 = (regs[2] & (1 << 9)) != 0;
        info->has_sse41 = (regs[2] & (1 << 19)) != 0;
        info->has_sse42 = (regs[2] & (1 << 20)) != 0;
        info->has_avx   = (regs[2] & (1 << 28)) != 0;
        info->has_fma   = (regs[2] & (1 << 12)) != 0;
    }
    
    /* Extended feature flags (leaf 7) */
    if (max_leaf >= 7) {
        CPUID_EX(regs, 7, 0);
        info->has_avx2     = (regs[1] & (1 << 5)) != 0;
        info->has_avx512f  = (regs[1] & (1 << 16)) != 0;
        info->has_avx512dq = (regs[1] & (1 << 17)) != 0;
        info->has_avx512bw = (regs[1] & (1 << 30)) != 0;
        info->has_avx512vl = (regs[1] & (1 << 31)) != 0;
    }
    
    /* Cache info (leaf 0x80000006) */
    CPUID(regs, 0x80000006);
    info->cache_line_size = regs[2] & 0xFF;
    info->l2_cache = (regs[2] >> 16) & 0xFFFF;  /* KB */
    
    /* L1 cache (leaf 0x80000005 - AMD, or use defaults) */
    CPUID(regs, 0x80000005);
    info->l1_data_cache = (regs[2] >> 24) & 0xFF;
    info->l1_inst_cache = (regs[3] >> 24) & 0xFF;
    
    /* L3 cache (leaf 0x80000006) */
    info->l3_cache = ((regs[3] >> 18) & 0x3FFF) * 512;  /* In 512KB units */
    
    /* Defaults if not detected */
    if (info->cache_line_size == 0) info->cache_line_size = 64;
    if (info->l1_data_cache == 0) info->l1_data_cache = 32;
    if (info->l2_cache == 0) info->l2_cache = 256;
    
    /* ARM features not available on x86 */
    info->has_neon = 0;
    info->has_sve = 0;
    info->has_sve2 = 0;
    info->sve_vector_length = 0;
}

#endif /* ARCH_X64 || ARCH_X86 */

/* ==========================================
 * ARM Feature Detection
 * ========================================== */

#if defined(ARCH_ARM64) || defined(ARCH_ARM32)

static void detect_arm_features(struct cpu_info* info) {
    strcpy(info->vendor, "ARM");
    strcpy(info->brand, "ARM Processor");
    
    /* x86 features not available */
    info->has_sse2 = 0;
    info->has_sse3 = 0;
    info->has_ssse3 = 0;
    info->has_sse41 = 0;
    info->has_sse42 = 0;
    info->has_avx = 0;
    info->has_avx2 = 0;
    info->has_fma = 0;
    info->has_avx512f = 0;
    info->has_avx512dq = 0;
    info->has_avx512vl = 0;
    info->has_avx512bw = 0;
    
    /* NEON: standard on ARM64, detect on ARM32 */
#if SIMD_NEON
    info->has_neon = 1;
#else
    info->has_neon = 0;
#endif
    
    /* SVE detection */
#if SIMD_SVE
    info->has_sve = 1;
    /* Get SVE vector length via svcntb() if available */
    #if defined(__ARM_FEATURE_SVE)
        info->sve_vector_length = svcntb() * 8;  /* Convert bytes to bits */
    #else
        info->sve_vector_length = 256;  /* Default assumption */
    #endif
#else
    info->has_sve = 0;
    info->sve_vector_length = 0;
#endif
    
#if SIMD_SVE2
    info->has_sve2 = 1;
#else
    info->has_sve2 = 0;
#endif
    
    /* Common ARM cache defaults */
    info->cache_line_size = 64;
    info->l1_data_cache = 32;
    info->l1_inst_cache = 32;
    info->l2_cache = 512;
    info->l3_cache = 0;
    
    /* Try to read cache info from sysfs on Linux */
#if defined(PLATFORM_LINUX)
    FILE* f = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
    if (f) {
        fscanf(f, "%d", &info->cache_line_size);
        fclose(f);
    }
#endif
}

#endif /* ARCH_ARM64 || ARCH_ARM32 */

/* ==========================================
 * RISC-V Feature Detection
 * ========================================== */

#if defined(ARCH_RISCV64) || defined(ARCH_RISCV32)

static void detect_riscv_features(struct cpu_info* info) {
    strcpy(info->vendor, "RISC-V");
    strcpy(info->brand, "RISC-V Processor");
    
    /* x86/ARM features not available */
    info->has_sse2 = 0;
    info->has_sse3 = 0;
    info->has_ssse3 = 0;
    info->has_sse41 = 0;
    info->has_sse42 = 0;
    info->has_avx = 0;
    info->has_avx2 = 0;
    info->has_fma = 0;
    info->has_avx512f = 0;
    info->has_avx512dq = 0;
    info->has_avx512vl = 0;
    info->has_avx512bw = 0;
    info->has_neon = 0;
    info->has_sve = 0;
    info->has_sve2 = 0;
    info->sve_vector_length = 0;
    
    /* Common RISC-V cache defaults */
    info->cache_line_size = 64;
    info->l1_data_cache = 32;
    info->l1_inst_cache = 32;
    info->l2_cache = 256;
    info->l3_cache = 0;
    
#if defined(PLATFORM_LINUX)
    /* Try to read from /proc/cpuinfo */
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "uarch", 5) == 0) {
                char* colon = strchr(line, ':');
                if (colon) {
                    strncpy(info->brand, colon + 2, sizeof(info->brand) - 1);
                    /* Remove newline */
                    char* nl = strchr(info->brand, '\n');
                    if (nl) *nl = '\0';
                }
            }
        }
        fclose(f);
    }
#endif
}

#endif /* ARCH_RISCV64 || ARCH_RISCV32 */

/* ==========================================
 * PowerPC Feature Detection
 * ========================================== */

#if defined(ARCH_PPC64) || defined(ARCH_PPC32)

static void detect_ppc_features(struct cpu_info* info) {
    strcpy(info->vendor, "IBM/Power");
    strcpy(info->brand, "PowerPC Processor");
    
    /* x86/ARM features not available */
    info->has_sse2 = 0;
    info->has_sse3 = 0;
    info->has_ssse3 = 0;
    info->has_sse41 = 0;
    info->has_sse42 = 0;
    info->has_avx = 0;
    info->has_avx2 = 0;
    info->has_fma = 0;
    info->has_avx512f = 0;
    info->has_avx512dq = 0;
    info->has_avx512vl = 0;
    info->has_avx512bw = 0;
    info->has_neon = 0;
    info->has_sve = 0;
    info->has_sve2 = 0;
    info->sve_vector_length = 0;
    
    /* Common PowerPC cache defaults */
    info->cache_line_size = 128;  /* POWER9/10 use 128-byte lines */
    info->l1_data_cache = 32;
    info->l1_inst_cache = 32;
    info->l2_cache = 512;
    info->l3_cache = 0;
    
#if defined(PLATFORM_LINUX)
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "cpu", 3) == 0 && line[3] == '\t') {
                char* colon = strchr(line, ':');
                if (colon) {
                    strncpy(info->brand, colon + 2, sizeof(info->brand) - 1);
                    char* nl = strchr(info->brand, '\n');
                    if (nl) *nl = '\0';
                }
            }
        }
        fclose(f);
    }
#endif
}

#endif /* ARCH_PPC64 || ARCH_PPC32 */

/* ==========================================
 * Generic/Other Architecture Detection
 * ========================================== */

#if !defined(ARCH_X64) && !defined(ARCH_X86) && !defined(ARCH_ARM64) && \
    !defined(ARCH_ARM32) && !defined(ARCH_RISCV64) && !defined(ARCH_RISCV32) && \
    !defined(ARCH_PPC64) && !defined(ARCH_PPC32)

static void detect_generic_features(struct cpu_info* info) {
    strcpy(info->vendor, ARCH_NAME);
    strcpy(info->brand, ARCH_NAME " Processor");
    
    /* No specific SIMD features detected */
    info->has_sse2 = 0;
    info->has_sse3 = 0;
    info->has_ssse3 = 0;
    info->has_sse41 = 0;
    info->has_sse42 = 0;
    info->has_avx = 0;
    info->has_avx2 = 0;
    info->has_fma = 0;
    info->has_avx512f = 0;
    info->has_avx512dq = 0;
    info->has_avx512vl = 0;
    info->has_avx512bw = 0;
    info->has_neon = 0;
    info->has_sve = 0;
    info->has_sve2 = 0;
    info->sve_vector_length = 0;
    
    /* Generic cache defaults */
    info->cache_line_size = 64;
    info->l1_data_cache = 32;
    info->l1_inst_cache = 32;
    info->l2_cache = 256;
    info->l3_cache = 0;
}

#endif /* Generic architecture */

/* ==========================================
 * Core Count Detection
 * ========================================== */

static void detect_core_count(struct cpu_info* info) {
#if defined(PLATFORM_WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    info->num_logical_cores = (int)si.dwNumberOfProcessors;
    /* Physical cores harder to get on Windows, use logical as estimate */
    info->num_physical_cores = info->num_logical_cores;
#elif defined(PLATFORM_MACOS)
    size_t size = sizeof(info->num_physical_cores);
    sysctlbyname("hw.physicalcpu", &info->num_physical_cores, &size, NULL, 0);
    size = sizeof(info->num_logical_cores);
    sysctlbyname("hw.logicalcpu", &info->num_logical_cores, &size, NULL, 0);
#elif defined(PLATFORM_LINUX)
    info->num_logical_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    /* Try to get physical cores from /proc/cpuinfo */
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        int cores = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "cpu cores", 9) == 0) {
                sscanf(line, "cpu cores : %d", &cores);
                break;
            }
        }
        fclose(f);
        info->num_physical_cores = cores > 0 ? cores : info->num_logical_cores;
    } else {
        info->num_physical_cores = info->num_logical_cores;
    }
#else
    info->num_physical_cores = 1;
    info->num_logical_cores = 1;
#endif
    
    /* Sanity check */
    if (info->num_physical_cores <= 0) info->num_physical_cores = 1;
    if (info->num_logical_cores <= 0) info->num_logical_cores = 1;
}

/* ==========================================
 * Public API
 * ========================================== */

const struct cpu_info* cpu_get_info(void) {
    if (!g_cpu_info_init) {
        detect_core_count(&g_cpu_info);
        
#if defined(ARCH_X64) || defined(ARCH_X86)
        detect_x86_features(&g_cpu_info);
#elif defined(ARCH_ARM64) || defined(ARCH_ARM32)
        detect_arm_features(&g_cpu_info);
#elif defined(ARCH_RISCV64) || defined(ARCH_RISCV32)
        detect_riscv_features(&g_cpu_info);
#elif defined(ARCH_PPC64) || defined(ARCH_PPC32)
        detect_ppc_features(&g_cpu_info);
#else
        detect_generic_features(&g_cpu_info);
#endif
        
        g_cpu_info_init = 1;
    }
    return &g_cpu_info;
}

int cpu_num_cores(void) {
    return cpu_get_info()->num_physical_cores;
}

int cpu_cache_line(void) {
    return cpu_get_info()->cache_line_size;
}

int cpu_has_sse2(void)   { return cpu_get_info()->has_sse2; }
int cpu_has_avx(void)    { return cpu_get_info()->has_avx; }
int cpu_has_avx2(void)   { return cpu_get_info()->has_avx2; }
int cpu_has_avx512(void) { return cpu_get_info()->has_avx512f; }
int cpu_has_neon(void)   { return cpu_get_info()->has_neon; }
int cpu_has_sve(void)    { return cpu_get_info()->has_sve; }

