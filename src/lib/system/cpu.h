#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include "../macros.h"
#include <stdint.h>

/* ==========================================
 * CPU Information
 * 
 * Runtime detection of CPU features.
 * Complements compile-time detection in macros.h
 * ========================================== */

/* ==========================================
 * CPU INFO STRUCTURE
 * ========================================== */

struct cpu_info {
    /* Core/thread count */
    int num_physical_cores;
    int num_logical_cores;
    
    /* Cache info (KB) */
    int cache_line_size;
    int l1_data_cache;
    int l1_inst_cache;
    int l2_cache;
    int l3_cache;
    
    /* Identification */
    char vendor[16];
    char brand[64];
    
    /* x86 SIMD features */
    int has_sse2;
    int has_sse3;
    int has_ssse3;
    int has_sse41;
    int has_sse42;
    int has_avx;
    int has_avx2;
    int has_fma;
    int has_avx512f;
    int has_avx512dq;
    int has_avx512vl;
    int has_avx512bw;
    
    /* ARM SIMD features */
    int has_neon;
    int has_sve;
    int has_sve2;
    uint32_t sve_vector_length;  /* SVE vector length in bits (0 if no SVE) */
    
};

/* ==========================================
 * API (implementation in cpu.c)
 * ========================================== */

/* Get CPU info (cached after first call) */
const struct cpu_info* cpu_get_info(void);

/* Quick accessors */
int cpu_num_cores(void);
int cpu_cache_line(void);

/* SIMD availability (runtime check) */
int cpu_has_sse2(void);
int cpu_has_avx(void);
int cpu_has_avx2(void);
int cpu_has_avx512(void);
int cpu_has_neon(void);
int cpu_has_sve(void);

/* Log CPU info */
void cpu_log_info(void);

/* ==========================================
 * SYSTEM MEMORY INFO
 * ========================================== */

/* Platform includes for memory/system queries */
#if defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    #include <sys/sysinfo.h>
    #include <unistd.h>
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#elif defined(POSIX)
    #include <unistd.h>
#else
    #warning "Unknown OS, limited memory info available"
#endif

/* Get total system RAM in bytes */
static force_inline uint64_t get_total_memory(void) {
#if defined(PLATFORM_WINDOWS)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return ms.ullTotalPhys;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    struct sysinfo si;
    sysinfo(&si);
    return (uint64_t)si.totalram * si.mem_unit;
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    uint64_t mem;
    size_t size = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &size, NULL, 0);
    return mem;
#elif defined(POSIX)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGESIZE);
    return (uint64_t)pages * page_size;
#else
    return 0;
#endif
}

/* Get available system RAM in bytes */
static force_inline uint64_t get_available_memory(void) {
#if defined(PLATFORM_WINDOWS)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return ms.ullAvailPhys;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    struct sysinfo si;
    sysinfo(&si);
    return (uint64_t)si.freeram * si.mem_unit;
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    mach_port_t host = mach_host_self();
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(integer_t);
    host_page_size(host, &page_size);
    host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count);
    return (uint64_t)vm_stat.free_count * page_size;
#elif defined(POSIX)
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGESIZE);
    return (uint64_t)pages * page_size;
#else
    return 0;
#endif
}

/* Get system page size */
static force_inline size_t get_page_size(void) {
#if defined(PLATFORM_WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#elif defined(POSIX)
    return (size_t)sysconf(_SC_PAGESIZE);
#else
    return 4096;  /* Common default */
#endif
}

#endif /* CPU_H_INCLUDED */
