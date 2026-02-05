#include "lib/macros.h"
#include <stdio.h>
#include <stdlib.h>
#include "core/vm.h"

// Helper to encode instructions
static inline uint32_t encode_r_type(uint8_t opcode, uint8_t rd, uint8_t rn, uint8_t rm, uint16_t funct) {
    return ((uint32_t)opcode << 26) | ((uint32_t)rd << 21) | ((uint32_t)rn << 16) | 
           ((uint32_t)rm << 11) | funct;
}

static inline uint32_t encode_i_type(uint8_t opcode, uint8_t rd, uint8_t rn, uint16_t imm) {
    return ((uint32_t)opcode << 26) | ((uint32_t)rd << 21) | ((uint32_t)rn << 16) | imm;
}

int main(void) {
    VM vm;
    vm_init(&vm);
    
    printf("=== SIVM Test Suite ===\n\n");
    
    // Test program: compute 5 + 3 = 8
    // ADDI r1, r0, 5     ; r1 = 5
    // ADDI r2, r0, 3     ; r2 = 3
    // ALU  r3, r1, r2, ADD  ; r3 = r1 + r2
    // HALT
    
    uint32_t program[] = {
        encode_i_type(OP_ADDI, 1, 0, 5),     // r1 = 5
        encode_i_type(OP_ADDI, 2, 0, 3),     // r2 = 3
        encode_r_type(OP_ALU, 3, 1, 2, FUNCT_ADD),  // r3 = r1 + r2
        encode_i_type(OP_HALT, 0, 0, 0),    // halt
    };
    
    // Load program at address 0
    if (vm_load_program(&vm, (uint8_t*)program, sizeof(program), 0) != 0) {
        fprintf(stderr, "Failed to load program\n");
        return 1;
    }
    
    vm_set_pc(&vm, 0, 0);
    
    printf("Test 1: Basic arithmetic (5 + 3)\n");
    printf("Before execution:\n");
    printf("  r1 = %lu\n", vm.core[0].reg[1]);
    printf("  r2 = %lu\n", vm.core[0].reg[2]);
    printf("  r3 = %lu\n", vm.core[0].reg[3]);
    
    // Run the VM
    run(&vm, 0);
    
    printf("After execution:\n");
    printf("  r1 = %lu (expected 5)\n", vm.core[0].reg[1]);
    printf("  r2 = %lu (expected 3)\n", vm.core[0].reg[2]);
    printf("  r3 = %lu (expected 8)\n", vm.core[0].reg[3]);
    printf("  Error code: %d (expected %d = VM_ERR_HALTED)\n", vm.core[0].error, VM_ERR_HALTED);
    
    if (vm.core[0].reg[3] == 8 && vm.core[0].error == VM_ERR_HALTED) {
        printf("  [PASS]\n\n");
    } else {
        printf("  [FAIL]\n\n");
        return 1;
    }
    
    // Test 2: Load/Store
    printf("Test 2: Load/Store operations\n");
    vm_init(&vm);  // Reset VM
    
    uint32_t program2[] = {
        encode_i_type(OP_ADDI, 1, 0, 0x1234),  // r1 = 0x1234
        encode_i_type(OP_ADDI, 2, 0, 0x100),   // r2 = 0x100 (address)
        encode_i_type(OP_SD, 1, 2, 0),         // store r1 at [r2+0]
        encode_i_type(OP_LD, 3, 2, 0),         // load r3 from [r2+0]
        encode_i_type(OP_HALT, 0, 0, 0),
    };
    
    vm_load_program(&vm, (uint8_t*)program2, sizeof(program2), 0);
    vm_set_pc(&vm, 0, 0);
    run(&vm, 0);
    
    printf("  r1 = 0x%lx\n", vm.core[0].reg[1]);
    printf("  r3 = 0x%lx (expected 0x1234)\n", vm.core[0].reg[3]);
    
    if (vm.core[0].reg[3] == 0x1234) {
        printf("  [PASS]\n\n");
    } else {
        printf("  [FAIL]\n\n");
        return 1;
    }
    
    // Test 3: Branch
    printf("Test 3: Branch operations\n");
    vm_init(&vm);
    
    // Branch offset is relative to PC of branch instruction, in units of 4 bytes
    // Branch at addr 8, want to jump to addr 20, so offset = (20-8)/4 = 3
    uint32_t program3[] = {
        encode_i_type(OP_ADDI, 1, 0, 10),      // 0: r1 = 10
        encode_i_type(OP_ADDI, 2, 0, 10),      // 4: r2 = 10
        encode_i_type(OP_BEQ, 1, 2, 3),        // 8: if r1 == r2, jump +3 instr (to addr 20)
        encode_i_type(OP_ADDI, 3, 0, 0),       // 12: r3 = 0 (should be skipped)
        encode_i_type(OP_HALT, 0, 0, 0),       // 16: halt (should be skipped)
        encode_i_type(OP_ADDI, 3, 0, 1),       // 20: r3 = 1 (should execute)
        encode_i_type(OP_HALT, 0, 0, 0),       // 24: halt
    };
    
    vm_load_program(&vm, (uint8_t*)program3, sizeof(program3), 0);
    vm_set_pc(&vm, 0, 0);
    run(&vm, 0);
    
    printf("  r3 = %lu (expected 1)\n", vm.core[0].reg[3]);
    
    if (vm.core[0].reg[3] == 1) {
        printf("  [PASS]\n\n");
    } else {
        printf("  [FAIL]\n\n");
        return 1;
    }
    
    // Test 4: Multiplication
    printf("Test 4: Multiplication\n");
    vm_init(&vm);
    
    uint32_t program4[] = {
        encode_i_type(OP_ADDI, 1, 0, 7),       // r1 = 7
        encode_i_type(OP_ADDI, 2, 0, 6),       // r2 = 6
        encode_r_type(OP_ALU, 3, 1, 2, FUNCT_MUL),  // r3 = r1 * r2
        encode_i_type(OP_HALT, 0, 0, 0),
    };
    
    vm_load_program(&vm, (uint8_t*)program4, sizeof(program4), 0);
    vm_set_pc(&vm, 0, 0);
    run(&vm, 0);
    
    printf("  r3 = %lu (expected 42)\n", vm.core[0].reg[3]);
    
    if (vm.core[0].reg[3] == 42) {
        printf("  [PASS]\n\n");
    } else {
        printf("  [FAIL]\n\n");
        return 1;
    }
    
    printf("=== All tests passed! ===\n");
    return 0;
}