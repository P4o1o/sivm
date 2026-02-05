/* ==========================================
 * Threading Primitives
 * 
 * Cross-platform threads, mutexes, conditions.
 * Wraps pthreads (PLATFORM_POSIX) and Win32 threads.
 * 
 * Requires: macros.h for platform detection
 * ========================================== */

#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

/* Include macros.h first - it defines _PLATFORM_POSIX_C_SOURCE before system headers */
#include "../macros.h"

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

/* ==========================================
 * PLATFORM INCLUDES
 * ========================================== */
#if defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(PLATFORM_POSIX)
    #include <pthread.h>
    #include <sched.h>
    #include <time.h>
#elif defined(PLATFORM_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    #include "event_groups.h"
#elif defined(PLATFORM_ZEPHYR)
    #include <zephyr/kernel.h>
#else
    #error "Unsupported platform for threading. Define PLATFORM_WINDOWS, PLATFORM_POSIX, PLATFORM_FREERTOS, or PLATFORM_ZEPHYR."
#endif

/* ==========================================
 * TYPES
 * ========================================== */

#if defined(PLATFORM_WINDOWS)
    typedef HANDLE thread;
    typedef CRITICAL_SECTION mutex;
    typedef CONDITION_VARIABLE condition;
    typedef SRWLOCK rwlock;
    typedef DWORD thread_ret;
    #define THREAD_CALL __stdcall
    #if defined(COMPILER_MSVC)
        #define THREAD_LOCAL __declspec(thread)
    #elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
        #define THREAD_LOCAL __thread
    #else
        #define THREAD_LOCAL  /* Not supported */
    #endif
#elif defined(PLATFORM_POSIX)
    typedef pthread_t thread;
    typedef pthread_mutex_t mutex;
    typedef pthread_cond_t condition;
    /* RWLock wrapper for PLATFORM_POSIX (custom for wider compatibility) */
    typedef struct {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        int readers;
        int writers;
    } rwlock;
    typedef void* thread_ret;
    #define THREAD_CALL
    #define THREAD_LOCAL __thread
#elif defined(PLATFORM_FREERTOS)
    typedef TaskHandle_t thread;
    typedef SemaphoreHandle_t mutex;
    typedef SemaphoreHandle_t condition;  /* Binary semaphore for signaling */
    typedef struct {
        SemaphoreHandle_t mutex;
        SemaphoreHandle_t write_sem;
        volatile int readers;
    } rwlock;
    typedef void thread_ret;
    #define THREAD_CALL
    #define THREAD_LOCAL  /* Use pvTaskGetThreadLocalStoragePointer instead */
    
    /* FreeRTOS thread config for create */
    #ifndef THREAD_STACK_SIZE
        #define THREAD_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)
    #endif
    #ifndef THREAD_PRIORITY
        #define THREAD_PRIORITY (tskIDLE_PRIORITY + 1)
    #endif
#elif defined(PLATFORM_ZEPHYR)
    /* Zephyr thread wrapper with embedded stack */
    #ifndef ZEPHYR_THREAD_STACK_SIZE
        #define ZEPHYR_THREAD_STACK_SIZE 2048
    #endif
    typedef struct {
        struct k_thread thread;
        k_thread_stack_t *stack;
        k_tid_t tid;
    } thread;
    typedef struct k_mutex mutex;
    typedef struct k_condvar condition;
    typedef struct {
        struct k_mutex mutex;
        struct k_condvar cond;
        volatile int readers;
        volatile int writers;
    } rwlock;
    typedef void thread_ret;
    #define THREAD_CALL
    #define THREAD_LOCAL  /* Use k_thread_custom_data_set/get instead */
#endif

typedef thread_ret (THREAD_CALL *thread_func)(void* arg);

/* ==========================================
 * THREAD API
 * ========================================== */

/* Create and start thread */
static force_inline int thread_create(thread* t, thread_func func, void* arg) {
    int result = 0;
#if defined(PLATFORM_WINDOWS)
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
    result = (*t != NULL) ? 0 : -1;
#elif defined(PLATFORM_POSIX)
    result = pthread_create(t, NULL, func, arg);
#elif defined(PLATFORM_FREERTOS)
    BaseType_t ret = xTaskCreate((TaskFunction_t)func, "thread", THREAD_STACK_SIZE, arg, THREAD_PRIORITY, t);
    result = (ret == pdPASS) ? 0 : -1;
#elif defined(PLATFORM_ZEPHYR)
    /* Allocate stack from heap - requires CONFIG_HEAP_MEM_POOL_SIZE > 0 */
    t->stack = k_thread_stack_alloc(ZEPHYR_THREAD_STACK_SIZE, 0);
    if (t->stack == NULL) {
        return -1;
    }
    t->tid = k_thread_create(&t->thread, t->stack, ZEPHYR_THREAD_STACK_SIZE,
                             (k_thread_entry_t)func, arg, NULL, NULL,
                             K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
    result = (t->tid != NULL) ? 0 : -1;
#endif
    return result;
}

/* Wait for thread completion */
static force_inline void thread_join(thread t) {
#if defined(PLATFORM_WINDOWS)
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
#elif defined(PLATFORM_POSIX)
    pthread_join(t, NULL);
#elif defined(PLATFORM_FREERTOS)
    /* FreeRTOS doesn't have join - wait for task to delete itself */
    while (eTaskGetState(t) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
#elif defined(PLATFORM_ZEPHYR)
    k_thread_join(&t.thread, K_FOREVER);
    /* Free the allocated stack */
    if (t.stack != NULL) {
        k_thread_stack_free(t.stack);
    }
#endif
}

/* Detach thread (auto-cleanup on exit) */
static force_inline void thread_detach(thread t) {
#if defined(PLATFORM_WINDOWS)
    CloseHandle(t);
#elif defined(PLATFORM_POSIX)
    pthread_detach(t);
#elif defined(PLATFORM_FREERTOS)
    (void)t;  /* FreeRTOS tasks auto-cleanup when deleted */
#elif defined(PLATFORM_ZEPHYR)
    (void)t;  /* Zephyr threads can be detached via options */
#endif
}

/* Yield to other threads */
static force_inline void thread_yield(void) {
#if defined(PLATFORM_WINDOWS)
    SwitchToThread();
#elif defined(PLATFORM_POSIX)
    sched_yield();
#elif defined(PLATFORM_FREERTOS)
    taskYIELD();
#elif defined(PLATFORM_ZEPHYR)
    k_yield();
#endif
}

/* Get current thread ID */
static force_inline uint64_t thread_id(void) {
#if defined(PLATFORM_WINDOWS)
    return (uint64_t)GetCurrentThreadId();
#elif defined(PLATFORM_POSIX)
    return (uint64_t)pthread_self();
#elif defined(PLATFORM_FREERTOS)
    return (uint64_t)(uintptr_t)xTaskGetCurrentTaskHandle();
#elif defined(PLATFORM_ZEPHYR)
    return (uint64_t)(uintptr_t)k_current_get();
#endif
}

/* Set thread name (debugging) */
static force_inline void thread_set_name(const char* name) {
#if defined(PLATFORM_WINDOWS)
    /* Set thread name for Visual Studio debugger using exception */
    #if defined(COMPILER_MSVC)
    typedef struct {
        DWORD type;
        LPCSTR name;
        DWORD id;
        DWORD flags;
    } THREADNAME_INFO;
    THREADNAME_INFO info;
    info.type = 0x1000;
    info.name = name;
    info.id = GetCurrentThreadId();
    info.flags = 0;
    __try {
        RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    #else
    (void)name;  /* Non-MSVC on Windows - no SEH support */
    #endif
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    pthread_setname_np(pthread_self(), name);
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    pthread_setname_np(name);
#elif defined(PLATFORM_FREEBSD)
    pthread_set_name_np(pthread_self(), name);
#elif defined(PLATFORM_FREERTOS)
    (void)name;  /* FreeRTOS task names set at creation */
#elif defined(PLATFORM_ZEPHYR)
    k_thread_name_set(k_current_get(), name);
#else
    (void)name;
#endif
}

/* ==========================================
 * MUTEX API
 * ========================================== */

static force_inline void mutex_init(mutex* m) {
#if defined(PLATFORM_WINDOWS)
    InitializeCriticalSection(m);
#elif defined(PLATFORM_POSIX)
    int r = pthread_mutex_init(m, NULL);
    assert(r == 0);
    (void)r;
#elif defined(PLATFORM_FREERTOS)
    *m = xSemaphoreCreateMutex();
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_init(m);
#endif
}

static force_inline void mutex_destroy(mutex* m) {
#if defined(PLATFORM_WINDOWS)
    DeleteCriticalSection(m);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_destroy(m);
#elif defined(PLATFORM_FREERTOS)
    vSemaphoreDelete(*m);
#elif defined(PLATFORM_ZEPHYR)
    (void)m;  /* No destroy needed */
#endif
}

static force_inline void mutex_lock(mutex* m) {
#if defined(PLATFORM_WINDOWS)
    EnterCriticalSection(m);
#elif defined(PLATFORM_POSIX)
    int r = pthread_mutex_lock(m);
    assert(r == 0);
    (void)r;
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreTake(*m, portMAX_DELAY);
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_lock(m, K_FOREVER);
#endif
}

static force_inline void mutex_unlock(mutex* m) {
#if defined(PLATFORM_WINDOWS)
    LeaveCriticalSection(m);
#elif defined(PLATFORM_POSIX)
    int r = pthread_mutex_unlock(m);
    assert(r == 0);
    (void)r;
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreGive(*m);
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_unlock(m);
#endif
}

static force_inline bool mutex_trylock(mutex* m) {
#if defined(PLATFORM_WINDOWS)
    return TryEnterCriticalSection(m) != 0;
#elif defined(PLATFORM_POSIX)
    return pthread_mutex_trylock(m) == 0;
#elif defined(PLATFORM_FREERTOS)
    return xSemaphoreTake(*m, 0) == pdTRUE;
#elif defined(PLATFORM_ZEPHYR)
    return k_mutex_lock(m, K_NO_WAIT) == 0;
#endif
}

/* ==========================================
 * CONDITION VARIABLE API
 * ========================================== */

static force_inline void condition_init(condition* c) {
#if defined(PLATFORM_WINDOWS)
    InitializeConditionVariable(c);
#elif defined(PLATFORM_POSIX)
    pthread_cond_init(c, NULL);
#elif defined(PLATFORM_FREERTOS)
    *c = xSemaphoreCreateBinary();
#elif defined(PLATFORM_ZEPHYR)
    k_condvar_init(c);
#endif
}

static force_inline void condition_destroy(condition* c) {
#if defined(PLATFORM_WINDOWS)
    (void)c;  /* No cleanup needed */
#elif defined(PLATFORM_POSIX)
    pthread_cond_destroy(c);
#elif defined(PLATFORM_FREERTOS)
    vSemaphoreDelete(*c);
#elif defined(PLATFORM_ZEPHYR)
    (void)c;  /* No destroy needed */
#endif
}

static force_inline void condition_wait(condition* c, mutex* m) {
#if defined(PLATFORM_WINDOWS)
    SleepConditionVariableCS(c, m, INFINITE);
#elif defined(PLATFORM_POSIX)
    pthread_cond_wait(c, m);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreGive(*m);
    xSemaphoreTake(*c, portMAX_DELAY);
    xSemaphoreTake(*m, portMAX_DELAY);
#elif defined(PLATFORM_ZEPHYR)
    k_condvar_wait(c, m, K_FOREVER);
#endif
}

/* Wait with timeout (returns false if timed out) */
static force_inline bool condition_wait_timeout(condition* c, mutex* m, uint32_t timeout_ms) {
#if defined(PLATFORM_WINDOWS)
    return SleepConditionVariableCS(c, m, timeout_ms) != 0;
#elif defined(PLATFORM_POSIX)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(c, m, &ts) == 0;
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreGive(*m);
    BaseType_t ret = xSemaphoreTake(*c, pdMS_TO_TICKS(timeout_ms));
    xSemaphoreTake(*m, portMAX_DELAY);
    return ret == pdTRUE;
#elif defined(PLATFORM_ZEPHYR)
    return k_condvar_wait(c, m, K_MSEC(timeout_ms)) == 0;
#endif
}

static force_inline void condition_signal(condition* c) {
#if defined(PLATFORM_WINDOWS)
    WakeConditionVariable(c);
#elif defined(PLATFORM_POSIX)
    pthread_cond_signal(c);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreGive(*c);
#elif defined(PLATFORM_ZEPHYR)
    k_condvar_signal(c);
#endif
}

static force_inline void condition_broadcast(condition* c) {
#if defined(PLATFORM_WINDOWS)
    WakeAllConditionVariable(c);
#elif defined(PLATFORM_POSIX)
    pthread_cond_broadcast(c);
#elif defined(PLATFORM_FREERTOS)
    /* FreeRTOS doesn't have broadcast - signal once */
    xSemaphoreGive(*c);
#elif defined(PLATFORM_ZEPHYR)
    k_condvar_broadcast(c);
#endif
}

/* ==========================================
 * READ-WRITE LOCK API
 * Custom implementation using mutex+cond for PLATFORM_POSIX/embedded compatibility
 * ========================================== */

static force_inline void rwlock_init(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    InitializeSRWLock(rw);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_init(&rw->mutex, NULL);
    pthread_cond_init(&rw->cond, NULL);
    rw->readers = 0;
    rw->writers = 0;
#elif defined(PLATFORM_FREERTOS)
    rw->mutex = xSemaphoreCreateMutex();
    rw->write_sem = xSemaphoreCreateMutex();
    rw->readers = 0;
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_init(&rw->mutex);
    k_condvar_init(&rw->cond);
    rw->readers = 0;
    rw->writers = 0;
#endif
}

static force_inline void rwlock_destroy(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    (void)rw;  /* No cleanup needed */
#elif defined(PLATFORM_POSIX)
    pthread_mutex_destroy(&rw->mutex);
    pthread_cond_destroy(&rw->cond);
#elif defined(PLATFORM_FREERTOS)
    vSemaphoreDelete(rw->mutex);
    vSemaphoreDelete(rw->write_sem);
#elif defined(PLATFORM_ZEPHYR)
    (void)rw;  /* No destroy needed */
#endif
}

static force_inline void rwlock_rdlock(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    AcquireSRWLockShared(rw);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_lock(&rw->mutex);
    while (rw->writers > 0) {
        pthread_cond_wait(&rw->cond, &rw->mutex);
    }
    rw->readers++;
    pthread_mutex_unlock(&rw->mutex);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreTake(rw->write_sem, portMAX_DELAY);  /* Block writers */
    xSemaphoreTake(rw->mutex, portMAX_DELAY);
    rw->readers++;
    if (rw->readers == 1) {
        /* First reader, keep write_sem locked */
    } else {
        xSemaphoreGive(rw->write_sem);
    }
    xSemaphoreGive(rw->mutex);
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_lock(&rw->mutex, K_FOREVER);
    while (rw->writers > 0) {
        k_condvar_wait(&rw->cond, &rw->mutex, K_FOREVER);
    }
    rw->readers++;
    k_mutex_unlock(&rw->mutex);
#endif
}

static force_inline void rwlock_wrlock(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    AcquireSRWLockExclusive(rw);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_lock(&rw->mutex);
    while (rw->readers > 0 || rw->writers > 0) {
        pthread_cond_wait(&rw->cond, &rw->mutex);
    }
    rw->writers = 1;
    pthread_mutex_unlock(&rw->mutex);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreTake(rw->write_sem, portMAX_DELAY);
    /* Writers block on write_sem - readers must release it */
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_lock(&rw->mutex, K_FOREVER);
    while (rw->readers > 0 || rw->writers > 0) {
        k_condvar_wait(&rw->cond, &rw->mutex, K_FOREVER);
    }
    rw->writers = 1;
    k_mutex_unlock(&rw->mutex);
#endif
}

static force_inline void rwlock_rdunlock(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    ReleaseSRWLockShared(rw);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_lock(&rw->mutex);
    rw->readers--;
    if (rw->readers == 0) {
        pthread_cond_broadcast(&rw->cond);
    }
    pthread_mutex_unlock(&rw->mutex);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreTake(rw->mutex, portMAX_DELAY);
    rw->readers--;
    if (rw->readers == 0) {
        xSemaphoreGive(rw->write_sem);  /* Allow writers */
    }
    xSemaphoreGive(rw->mutex);
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_lock(&rw->mutex, K_FOREVER);
    rw->readers--;
    if (rw->readers == 0) {
        k_condvar_broadcast(&rw->cond);
    }
    k_mutex_unlock(&rw->mutex);
#endif
}

static force_inline void rwlock_wrunlock(rwlock* rw) {
#if defined(PLATFORM_WINDOWS)
    ReleaseSRWLockExclusive(rw);
#elif defined(PLATFORM_POSIX)
    pthread_mutex_lock(&rw->mutex);
    rw->writers = 0;
    pthread_cond_broadcast(&rw->cond);
    pthread_mutex_unlock(&rw->mutex);
#elif defined(PLATFORM_FREERTOS)
    xSemaphoreGive(rw->write_sem);
#elif defined(PLATFORM_ZEPHYR)
    k_mutex_lock(&rw->mutex, K_FOREVER);
    rw->writers = 0;
    k_condvar_broadcast(&rw->cond);
    k_mutex_unlock(&rw->mutex);
#endif
}

/* ==========================================
 * ONCE INITIALIZATION
 * ========================================== */

#if defined(PLATFORM_WINDOWS)
    typedef INIT_ONCE once_flag;
    #define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT
#elif defined(PLATFORM_POSIX)
    typedef pthread_once_t once_flag;
    #define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#elif defined(PLATFORM_FREERTOS)
    typedef volatile int once_flag;
    #define ONCE_FLAG_INIT 0
#elif defined(PLATFORM_ZEPHYR)
    typedef struct k_sem once_flag;
    #define ONCE_FLAG_INIT K_SEM_INITIALIZER(once_flag, 1, 1)
#endif

typedef void (*once_func)(void);

static force_inline void call_once(once_flag* once, once_func func) {
#if defined(PLATFORM_WINDOWS)
    BOOL pending;
    InitOnceBeginInitialize(once, 0, &pending, NULL);
    if (pending) {
        func();
        InitOnceComplete(once, 0, NULL);
    }
#elif defined(PLATFORM_POSIX)
    pthread_once(once, func);
#elif defined(PLATFORM_FREERTOS)
    if (*once == 0) {
        taskENTER_CRITICAL();
        if (*once == 0) {
            func();
            *once = 1;
        }
        taskEXIT_CRITICAL();
    }
#elif defined(PLATFORM_ZEPHYR)
    if (k_sem_take(once, K_NO_WAIT) == 0) {
        func();
    }
#endif
}

/* ==========================================
 * SLEEP UTILITIES
 * ========================================== */

static force_inline void thread_sleep_ms(uint32_t ms) {
#if defined(PLATFORM_WINDOWS)
    Sleep(ms);
#elif defined(PLATFORM_POSIX)
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#elif defined(PLATFORM_FREERTOS)
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(PLATFORM_ZEPHYR)
    k_msleep(ms);
#endif
}

#endif /* THREAD_H_INCLUDED */
