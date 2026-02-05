#ifndef SIVM_VM_H
#define SIVM_VM_H

// RISC64-style VM implementation

#include "../lib/macros.h"
#include "../lib/thread/thread.h"
#include "../lib/thread/atomic.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include "instruction.h"

#define REG_NUM      32
#define FREG_NUM     32
#define MEM_SIZE     (1 << 20)  // 1MB di memoria
#define CORE_NUM     4
#define STACK_SIZE   8192

// Special registers
#define REG_ZERO     0   // Sempre zero (hardwired)
#define REG_RA       1   // Return address
#define REG_SP       2   // Stack pointer
#define REG_GP       3   // Global pointer
#define REG_FP       30  // Frame pointer
#define REG_LINK     31  // Link register

// Condition flags
#define FLAG_ZERO     (1 << 0)
#define FLAG_NEGATIVE (1 << 1)
#define FLAG_CARRY    (1 << 2)
#define FLAG_OVERFLOW (1 << 3)
#define FLAG_HALTED   (1 << 15)

typedef uint64_t address;
typedef uint32_t instr;

// Forward declarations
struct VM;
struct Core;

struct Core {
    uint64_t reg[REG_NUM];      // General purpose registers
    double   freg[FREG_NUM];    // Floating point registers
    uint16_t status;            // Status register
    uint16_t flags;             // Condition flags
    address  pc;                // Program counter
    address  reservation;       // For LR/SC atomic operations
    bool     reservation_valid;
    uint64_t cycles;            // Cycle counter
    uint64_t instret;           // Instructions retired
    jmp_buf  jmp;               // For exception handling
    uint16_t error;             // Error code
};

typedef struct Core Core;

struct VM {
    Core        core[CORE_NUM];
    uint8_t     mem[MEM_SIZE];
    atomic_flag mem_lock;       // Per operazioni atomiche
    bool        running;
    // I/O callback
    int (*syscall_handler)(struct VM* vm, uint64_t core_num, uint64_t syscall_num);
};

typedef struct VM VM;

typedef enum {
    VM_OK = 0,
    VM_SYSCALL,
    VM_ERR_INVALID_ADDR,
    VM_ERR_INVALID_OPCODE,
    VM_ERR_DIV_BY_ZERO,
    VM_ERR_HALTED,
    VM_ERR_BREAKPOINT,
    VM_ERR_SYSCALL,
} VMSignal;

// Function declarations
void vm_init(struct VM *vm);
void run(struct VM *env, uint64_t core_num);
int  vm_load_program(struct VM *vm, const uint8_t *program, size_t size, address load_addr);
void vm_set_pc(struct VM *vm, uint64_t core_num, address pc);

#endif //SIVM_VM_H