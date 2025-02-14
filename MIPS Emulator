data, files, descriptors);
    }
}

/**
 * Loads a byte from a given valid memory address into the target register.
 */
static void lb_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, BYTE_LEN);

    int index = address - MEMORY_START;
    if (target != ZERO_REGISTER) {
        uint32_t mem_extract = 0;
        mem_extract = executable->initial_data[index];
        if ((mem_extract >> UINT8_SHIFT) & SIGN_BIT_MASK) {
            mem_extract -= UINT8_EXTENSION;
        }
        registers[target] = mem_extract;
    }
    data->index++;
}

/**
 * Loads a half word from a given valid memory address into the target register.
 */
static void lh_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, HALF_WORD_LEN);

    int index = address - MEMORY_START;
    if (target != ZERO_REGISTER) {
        uint32_t mem_extract = 0;
        for (int i = 0; i < HALF_WORD_LEN; i++) {
            mem_extract |= 
                executable->initial_data[index + i] << (BYTE_SIZE * i);
        }
        if ((mem_extract >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
            mem_extract -= SIGN_BIT_EXTENSION;
        }
        registers[target] = mem_extract;
    }
    data->index++;
}

/**
 * Loads a word from a given valid memory address into the target register.
 */
static void lw_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }  
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, WORD_LEN);  

    int index = address - MEMORY_START;
    if (target != ZERO_REGISTER) {
        uint32_t mem_extract = 0;
        for (int i = 0; i < WORD_LEN; i++) {
            mem_extract |= 
                executable->initial_data[index + i] << (BYTE_SIZE * i);
        }
        registers[target] = mem_extract;
    }
    data->index++;      
}

/**
 * Saves a byte from the target register to a valid memory address.
 */
static void sb_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, BYTE_LEN);

    int index = address - MEMORY_START;
    executable->initial_data[index] = registers[target];
    data->index++;
}

/**
 * Saves a half word from the target register to a valid memory address.
 */
static void sh_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, HALF_WORD_LEN);

    int index = address - MEMORY_START;
    for (int i = 0; i < HALF_WORD_LEN; i++) {
        executable->initial_data[index + i] = (registers[target] >> 
            (BYTE_SIZE * i)) & UINT8_MASK;
    }
    data->index++;
}

/**
 * Saves a word from the target register to a valid memory address.
 */
static void sw_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable) {
    uint8_t base = (execute >> BASE_SHIFT) & REGISTER_MASK;                    
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    uint32_t address = registers[base] + offset;
    address_check(address, executable, WORD_LEN);

    int index = address - MEMORY_START;
    for (int i = 0; i < WORD_LEN; i++) {
        executable->initial_data[index + i] = (registers[target] >> 
            (BYTE_SIZE * i)) & UINT8_MASK;
    }
    data->index++;
}

/**
 * Checks and prints out any changes of values in registers. Used for tracing
 * in subset 4. 
 */
static void print_modified(struct runtime_data *data) {
    uint32_t *prev_registers = data->prev_registers;
    uint32_t *curr_registers = data->registers;
    int index = -1;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (prev_registers[i] != curr_registers[i]) {
            index = i;
            fprintf(stdout, "   ");
            if (i == 1) {
                fprintf(stdout, "$at: ");
            } else if (i >= 2 && i <= 3) {
                fprintf(stdout, "$v%d: ", i - 2);
            } else if (i >= 4 && i <= 7) {
                fprintf(stdout, "$a%d: ", i - 4);
            } else if (i >= 8 && i <= 15) {
                fprintf(stdout, "$t%d: ", i - 8);
            } else if (i >= 16 && i <= 23) {
                fprintf(stdout, "$s%d: ", i - 16);
            } else if (i >= 24 && i <= 25) {
                fprintf(stdout, "$t%d: ", i - 16);
            } else if (i >= 26 && i <= 27) {
                fprintf(stdout, "$k%d: ", i - 26);
            } else if (i == 28) {
                fprintf(stdout, "$gp: ");
            } else if (i == 29) {
                fprintf(stdout, "$sp: ");
            } else if (i == 30) {
                fprintf(stdout, "$fp: ");
            } else {
                fprintf(stdout, "$ra: ");
            }
        }
    }
    // If a register has been modified, print out the modification
    if (index != -1) {
        print_uint32_in_hexadecimal(stdout, prev_registers[index]);
        fprintf(stdout, " -> ");
        print_uint32_in_hexadecimal(stdout, curr_registers[index]);
        putchar('\n');
    }
}

// Printing out exact-width integers in a portable way is slightly tricky,
// since we can't assume that a uint32_t is an unsigned int or that a
// int32_t is an int. So we can't just use %x or %d. A solution is to use
// printf format specifiers from the <inttypes.h> header. The following two
// functions are provided for your convenience so that you just call them
// without worring about <inttypes.h>, although you don't have to use use them.

// Print out a 32 bit integer in hexadecimal, including the leading `0x`.
//
// @param stream The file stream to output to.
// @param value The 32 bit integer to output.
void print_uint32_in_hexadecimal(FILE *stream, uint32_t value) {
    fprintf(stream, "0x%08" PRIx32, value);
}

// Print out a signed 32 bit integer in decimal.
//
// @param stream The file stream to output to.
// @param value The 32 bit integer to output.
void print_int32_in_decimal(FILE *stream, int32_t value) {
    fprintf(stream, "%" PRIi32, value);
}
