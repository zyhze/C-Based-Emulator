////////////////////////////////////////////////////////////////////////
// Project: `imps', a simple MIPS emulator
//
// This program implements some basic functions that seek to emulate a mips 
// assembly program, including arithmetic, branching, memory and file access 
// as well as system calls. 
//
// Written by Ray Xu on 14/11/2024.
//
// 2024-10-25   v1.0    


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

// #defines used for determining and executing instructions
#define UINT16_MASK 0xFFFF
#define UINT8_MASK 0xFF
#define REGISTER_MASK 0x1F
#define MAGIC_NUM_SIZE 4 
#define MAGIC_BYTE_0 0x49
#define MAGIC_BYTE_1 0x4D
#define MAGIC_BYTE_2 0x50
#define MAGIC_BYTE_3 0x53
#define INSTRUCTIONS_LEN 4
#define ENTRY_POINT_LEN 4
#define DEBUG_OFFSET_LEN 4
#define MEMORY_SIZE_LEN 2
#define OPCODE_SHIFT 26
#define OPCODE_MASK 0x3F
#define ADDI_INST 0x08
#define FUNCT_CHECK 0x00
#define ORI_INST 0x0D
#define LUI_INST 0x0F
#define ADDIU_INST 0x09
#define MUL_INST 0x1C
#define BEQ_INST 0x04
#define BNE_INST 0x05
#define SOURCE_SHIFT 21
#define TARGET_SHIFT 16
#define DESTINATION_SHIFT 11
#define BASE_SHIFT 21
#define IMMEDIATE_MASK 0xFFFF
#define OFFSET_MASK 0xFFFF
#define SIGN_BIT_SHIFT 15
#define SIGN_BIT_MASK 1
#define SIGN_BIT_EXTENSION 0x10000
#define UINT8_SHIFT 7
#define UINT8_EXTENSION 0x100
#define LUI_SHIFT 16
#define ZERO_REGISTER 0x00
#define FUNCT_MASK 0x3F
#define SYSCALL_INST 0x0C
#define ADD_INST 0x20
#define CLO_INST 0x11
#define CLZ_INST 0x10
#define ADDU_INST 0x21
#define SLT_INST 0x2A
#define LB_INST 0x20
#define LH_INST 0x21
#define LW_INST 0x23
#define SB_INST 0x28
#define SH_INST 0x29
#define SW_INST 0x2B

// #defines for syscalls
#define SYSCALL_1 1
#define SYSCALL_4 4
#define SYSCALL_10 10
#define SYSCALL_11 11
#define SYSCALL_12 12
#define SYSCALL_13 13
#define SYSCALL_14 14
#define SYSCALL_15 15
#define SYSCALL_16 16

// #defines for registers, memory and file manipulation
#define BYTE_SIZE 8
#define V0 2
#define A0 4
#define A1 5
#define A2 6
#define MEMORY_START 0x10010000
#define BYTE_LEN 1
#define HALF_WORD_LEN 2
#define WORD_LEN 4
#define NUM_REGISTERS 32
#define MAX_FILE_SIZE 128
#define MAX_FILE_NUM 6
#define MAX_DESC_NUM 8 


// Do not rename or modify this struct! It's directly used
// by the subset 1 autotests.

struct imps_file {
    uint32_t num_instructions;
    uint32_t entry_point;
    uint32_t *instructions;
    uint32_t *debug_offsets;
    uint16_t memory_size;
    uint8_t *initial_data;
};

// Used to keep track of all registers, a previous iteration of all 
// registers before an instruction and the index to determine which 
// instruction the program is up to.
struct runtime_data {
    uint32_t *registers;
    // This is a copy of the registers before an instruction is performed
    // used for trace mode to check for any changes made. 
    uint32_t *prev_registers;
    uint32_t index;
};

// File struct to emulate an in memory file system
struct file {
    char *path; // name of the file
    char data[MAX_FILE_SIZE]; // data stored in the file
    int size; // size of the file
};

// Descriptor struct to keep track of file access and position
struct descriptor {
    int file_index;
    int pos;
    bool read;
    bool write;
};

// Function prototypes used during implementation
void read_imps_file(char *path, struct imps_file *executable);

void execute_imps(struct imps_file *executable, int trace_mode, char *path);

void print_uint32_in_hexadecimal(FILE *stream, uint32_t value);

void print_int32_in_decimal(FILE *stream, int32_t value);

static void check_magic_number(FILE *input_stream);

static uint32_t get_lit_end_int(FILE *input_stream, int num_bytes);

static void initialise_files(struct file *files, 
                             struct descriptor *descriptors);

static void print_past_end(struct runtime_data *data, struct file *files, 
                           struct descriptor *descriptors);

static void free_data(struct runtime_data *data, struct file *files, 
                     struct descriptor *descriptors);

static void trace(struct runtime_data *data, struct imps_file *executable, 
                  char *path);

static void add_i_inst(uint32_t execute, struct runtime_data *data);

static void funct_check(uint32_t execute, struct runtime_data *data, 
                        struct imps_file *executable, struct file *files,
                        struct descriptor *descriptors);

static void overflow_check(int value1, int value2);

static void syscall(struct runtime_data *data, struct imps_file *executable, 
                    struct file *files, struct descriptor *descriptors);

static void print_string(struct runtime_data *data, 
                        struct imps_file *exectuable);

static void address_check(uint32_t address, struct imps_file *executable, 
                          int num_bytes);

static void read_char(struct runtime_data *data);

static void open_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable);

static uint32_t lowest_desc(struct descriptor *descriptors, int i, 
                            uint32_t type);

static void read_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable);

static void write_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable);

static void close_file(struct runtime_data *data, struct file *files,
                       struct descriptor *descriptors, 
                       struct imps_file *executable);

static void add_inst(uint32_t execute, struct runtime_data *data);

static void clo_inst(uint32_t execute, struct runtime_data *data);

static void clz_inst(uint32_t execute, struct runtime_data *data);

static void addu_inst(uint32_t execute, struct runtime_data *data);

static void slt_inst(uint32_t execute, struct runtime_data *data);

static void print_bad_instruction(uint32_t execute, struct runtime_data *data,
                                  struct file *files,
                                  struct descriptor *descriptors);

static void ori_inst(uint32_t execute, struct runtime_data *data);

static void lui_inst(uint32_t execute, struct runtime_data *data);

static void addiu_inst(uint32_t execute, struct runtime_data *data);

static void mul_inst(uint32_t execute, struct runtime_data *data);

static void beq_inst(uint32_t execute, struct runtime_data *data);

static void bne_inst(uint32_t execute, struct runtime_data *data);


static void mem_inst(uint32_t execute, struct runtime_data *data,
                     struct imps_file *executable, struct file *files,
                     struct descriptor *descriptors);

static void lb_inst(uint32_t execute, struct runtime_data *data, 
                    struct imps_file *executable);

static void lh_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable);

static void lw_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable);

static void sb_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable);

static void sh_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable);

static void sw_inst(uint32_t execute, struct runtime_data *data,
                    struct imps_file *executable);

static void print_modified(struct runtime_data *data);

/**
 * Main function to execute an IMPS emulator.
 */
int main(int argc, char *argv[]) {

    // Do NOT CHANGE this function
    // put your code in read_imps_file, execute_imps and your own functions

    char *pathname;
    int trace_mode = 0;

    if (argc == 2) {
        pathname = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "-t") == 0) {
        trace_mode = 1;
        pathname = argv[2];
    } else {
        fprintf(stderr, "Usage: imps [-t] <executable>\n");
        exit(EXIT_FAILURE);
    }

    struct imps_file executable = {0};
    read_imps_file(pathname, &executable);

    execute_imps(&executable, trace_mode, pathname);

    free(executable.debug_offsets);
    free(executable.instructions);
    free(executable.initial_data);

    return 0;
}

/**
 * Reads an IMPS exectuable file from the file at 'path' into 'executable'.
 * Exists the program if the file can't be accessed or is not well-formed.
 */
void read_imps_file(char *path, struct imps_file *executable) {
    FILE *input_stream = fopen(path, "r");
    if (input_stream == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }
    check_magic_number(input_stream);

    // Number of instructions
    uint32_t num_instructions = get_lit_end_int(input_stream, INSTRUCTIONS_LEN);
    executable->num_instructions = num_instructions;

    // Entry point, start of instructions
    executable->entry_point = get_lit_end_int(input_stream, ENTRY_POINT_LEN);

    // Store all instructions
    uint32_t *instructions = malloc(num_instructions * sizeof(uint32_t));
    for (int i = 0; i < num_instructions; i++) {
        instructions[i] = get_lit_end_int(input_stream, INSTRUCTIONS_LEN);
    }
    executable->instructions = instructions;

    // Store all debug offsets
    uint32_t *debug_offsets = malloc(num_instructions * sizeof(uint32_t));
    for (int i = 0; i < num_instructions; i++) {
        debug_offsets[i] = get_lit_end_int(input_stream, DEBUG_OFFSET_LEN);
    }
    executable->debug_offsets = debug_offsets;

    // Get the memory size
    executable->memory_size = 
        get_lit_end_int(input_stream, MEMORY_SIZE_LEN) & UINT16_MASK;   

    // Store all initial data
    uint8_t *initial_data = malloc(executable->memory_size);
    fread(initial_data, sizeof(uint8_t), executable->memory_size, input_stream);
    executable->initial_data = initial_data;

    fclose(input_stream);
}

/**
 * Checks the magic number of a given file to determine whether the file path
 * is valid. Exits if invalid with an error.
 */
static void check_magic_number(FILE *input_stream) {
    uint8_t valid_check[MAGIC_NUM_SIZE];
    if (fread(valid_check, 1, sizeof(valid_check), input_stream) != 
        MAGIC_NUM_SIZE || 
        valid_check[0] != MAGIC_BYTE_0 || valid_check[1] != MAGIC_BYTE_1 ||
        valid_check[2] != MAGIC_BYTE_2 || valid_check[3] != MAGIC_BYTE_3) {
            fprintf(stderr, "Invalid IMPS file\n");
            fclose(input_stream);
            exit(EXIT_FAILURE);
    }    
}

/**
 * Helper function used to return a little endian unsigned integer given a 
 * position in the file.
 */
static uint32_t get_lit_end_int(FILE *input_stream, int num_bytes) {
    uint32_t num = 0;
    for (int i = 0; i < num_bytes; i++) {
        num |= (uint32_t)fgetc(input_stream) << (BYTE_SIZE * i);
    }
    return num;
}

/**
 * Execute an IMPS program, determines required instructions and executes
 * corresponding required functions and memory access.
 */
void execute_imps(struct imps_file *executable, int trace_mode, char *path) {
    // Initialise register and run time data.
    struct runtime_data *data = malloc(sizeof(*data));
    data->registers = calloc(NUM_REGISTERS, sizeof(uint32_t));
    data->prev_registers = calloc(NUM_REGISTERS, sizeof(uint32_t));
    data->index = executable->entry_point;

    // Initialise file system in memory.
    struct file *files = malloc(MAX_FILE_NUM * sizeof(*files));
    struct descriptor *descriptors = 
        malloc(MAX_DESC_NUM * sizeof(*descriptors));
    initialise_files(files, descriptors);

    while (1) {
        if (data->index >= executable->num_instructions) {
            print_past_end(data, files, descriptors);
        }
        // If trace mode is on, make a copy of the registers.
        if (trace_mode == 1) {
            trace(data, executable, path);
            memcpy(data->prev_registers, data->registers, 
                NUM_REGISTERS * sizeof(uint32_t));
        }
        uint32_t execute = executable->instructions[data->index];
        uint8_t opcode = (execute >> OPCODE_SHIFT) & OPCODE_MASK;
        if (opcode == ADDI_INST) {
            add_i_inst(execute, data);
        } else if (opcode == FUNCT_CHECK) {
            funct_check(execute, data, executable, files, descriptors);
        } else if (opcode == ORI_INST) {
            ori_inst(execute, data);
        } else if (opcode == LUI_INST) {
            lui_inst(execute, data);
        } else if (opcode == ADDIU_INST) {
            addiu_inst(execute, data);
        } else if (opcode == MUL_INST) {
            mul_inst(execute, data);
        } else if (opcode == BEQ_INST) {
            beq_inst(execute, data);
        } else if (opcode == BNE_INST) {
            bne_inst(execute, data);
        } else {
            mem_inst(execute, data, executable, files, descriptors);
        }
        if (trace_mode == 1) {
            print_modified(data);
        }
    }
}

/**
 * Initialises in memory file system with values which are later used to 
 * open, read, write and close files.
 */
static void initialise_files(struct file *files, 
                             struct descriptor *descriptors) {
    for (int i = 0; i < MAX_FILE_NUM; i++) {
        files[i].path = NULL;
        files[i].size = 0;
        memset(files[i].data, 0, sizeof(files[i].data));
    }
    // Initialise descriptors
    for (int i = 0; i < MAX_DESC_NUM; i++) {
        descriptors[i].file_index = -1;
        descriptors[i].pos = 0;
        descriptors[i].read = false;
        descriptors[i].write = false;
    }
}

/**
 * If the end of the instructions array is accessed, an error should be 
 * produced and allocated memory is freed before exiting with status 1.
 */
static void print_past_end(struct runtime_data *data, struct file *files, 
                           struct descriptor *descriptors) {
    fprintf(stderr, "IMPS error: execution past the end of instructions\n");
    free_data(data, files, descriptors);
    exit(EXIT_FAILURE);
}

/**
 * Frees allocated memory for run time data, files and descriptors.
 */
static void free_data(struct runtime_data *data, struct file *files, 
                     struct descriptor *descriptors) {
    free(data->registers);
    free(data->prev_registers);
    free(data);
    for (int i = 0; i < MAX_FILE_NUM; i++) {
        free(files[i].path);
    }
    free(files);
    free(descriptors);
}

/**
 * Opens the corresponding assembly source file if it exists and prints out 
 * the instruction.
 */
static void trace(struct runtime_data *data, struct imps_file *executable, 
                  char *path) {
    // Find the mips file of the given path
    char *dot = strrchr(path, '.');
    strcpy(dot, ".s");
    FILE *trace_stream = fopen(path, "r");
    if (trace_stream == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }
    uint32_t offset = executable->debug_offsets[data->index];
    fseek(trace_stream, 0, SEEK_END);
    if (offset <= ftell(trace_stream)) {
        fseek(trace_stream, offset, SEEK_SET);
        int ch;
        while ((ch = fgetc(trace_stream)) != EOF && ch != '\n') {
            putchar(ch);
        }
        putchar('\n');
    }
    fclose(trace_stream);
}

/**
 * Stores the sum value of a register and an immediate value in the target 
 * register. Performs an overflow check.
 */
static void add_i_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t immediate = execute & IMMEDIATE_MASK;
    if ((immediate >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        immediate -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    if (target != ZERO_REGISTER) {
        overflow_check(immediate, registers[source]);   
        registers[target] = registers[source] + immediate;
    }
    data->index++;
}

/**
 * Checks if the addition of two values will result in an integer overflow.
 * Exits with status 1 if an overflow is detected.
 */
static void overflow_check(int value1, int value2) {
    if ((value1 > 0 && value2 > INT32_MAX - value1) || 
        (value1 < 0 && value2 < INT32_MIN - value1)) {
        fprintf(stderr, "IMPS error: addition would overflow\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Check the function bit pattern for instructions with the same opcode for an
 * r-type instruction.
 * If the instruction's function does not exist, then print an error.
 */
static void funct_check(uint32_t execute, struct runtime_data *data, 
                        struct imps_file *executable, struct file *files,
                        struct descriptor *descriptors) {
    uint8_t funct = execute & FUNCT_MASK;
    if (funct == SYSCALL_INST) {
        syscall(data, executable, files, descriptors);
    } else if (funct == ADD_INST) {
        add_inst(execute, data);
    } else if (funct == CLO_INST) {
        clo_inst(execute, data);
    } else if (funct == CLZ_INST) {
        clz_inst(execute, data);
    } else if (funct == ADDU_INST) {
        addu_inst(execute, data);
    } else if (funct == SLT_INST) {
        slt_inst(execute, data);
    } else {
        print_bad_instruction(execute, data, files, descriptors);
    }
}

/**
 * Executes the syscall determined by the value in the $v0 register.
 */
static void syscall(struct runtime_data *data, struct imps_file *executable, 
                    struct file *files, struct descriptor *descriptors) {
    if (data->registers[V0] == SYSCALL_1) {
        print_int32_in_decimal(stdout, data->registers[A0]);
    } else if (data->registers[V0] == SYSCALL_4) {
        print_string(data, executable);
    } else if (data->registers[V0] == SYSCALL_10) {
        free_data(data, files, descriptors);
        exit(EXIT_SUCCESS);
    } else if (data->registers[V0] == SYSCALL_11) {
        putchar(data->registers[A0]);
    } else if (data->registers[V0] == SYSCALL_12) {
        read_char(data);
    } else if (data->registers[V0] == SYSCALL_13) {
        open_file(data, files, descriptors, executable);
    } else if (data->registers[V0] == SYSCALL_14) {
        read_file(data, files, descriptors, executable);
    } else if (data->registers[V0] == SYSCALL_15) {
        write_file(data, files, descriptors, executable);
    } else if (data->registers[V0] == SYSCALL_16) {
        close_file(data, files, descriptors, executable);
    } else {
        fprintf(stderr, "IMPS error: bad syscall number\n");
        free_data(data, files, descriptors);
        exit(EXIT_FAILURE);
    }
    data->index++;
}

/**
 * Prints the nul-terminated string at address $a0 to sdout.
 */
static void print_string(struct runtime_data *data, 
                        struct imps_file *executable) {
    uint32_t address = data->registers[A0];
    address_check(address, executable, BYTE_LEN);
    int index = address - MEMORY_START;

    while (executable->initial_data[index] != '\0') {
        address_check(index + MEMORY_START, executable, BYTE_LEN);
        putchar(executable->initial_data[index]);
        index++;
    }                            
}

/**
 * Reads a single character from stdin and places that character in $v0.
 */
static void read_char(struct runtime_data *data) {
    uint32_t read_char = getchar();
    if (read_char == EOF) {
        data->registers[V0] = -1;
    } else {
        data->registers[V0] = read_char; 
    }    
}

/**
 * Checks if access of a particular size of data is valid in existing memory.
 */
static void address_check(uint32_t address, struct imps_file *executable, 
                          int num_bytes) {
    if (address < MEMORY_START || 
        address >= MEMORY_START + executable->memory_size + (num_bytes - 1) ||
        address % num_bytes != 0) {
        fprintf(stderr, "IMPS error: bad address for ");
        if (num_bytes == BYTE_LEN) {
            fprintf(stderr, "byte");
        } else if (num_bytes == HALF_WORD_LEN) {
            fprintf(stderr, "half");
        } else {
            fprintf(stderr, "word");
        }
        fprintf(stderr, " access: ");
        print_uint32_in_hexadecimal(stderr, address);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    } 
}

/**
 * Opens a file given a path name. If the file exists then it is assigned
 * the lowest available desciptor. If the file does not exist and is opened
 * for reading, then $v0 is set to -1, else it is set to write, and $v0 is 
 * set to the assigned desciptor.
 */
static void open_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable) {
    bool exists = false;
    uint32_t path_address = data->registers[A0];
    address_check(path_address, executable, BYTE_LEN);
    int path_index = path_address - MEMORY_START;

    // Get the path name string
    char ch = 0;
    char path_name[executable->memory_size];
    path_name[0] = '\0';
    while (executable->initial_data[path_index] != '\0') {
        ch = executable->initial_data[path_index];
        strncat(path_name, &ch, 1);
        path_index++;
    }
    // Find the file if it exists and assign lowest descriptor.
    for (int i = 0; i < MAX_FILE_NUM; i++) {
        if (files[i].path != NULL && strcmp(files[i].path, path_name) == 0) {
            exists = true;
            data->registers[V0] = 
                lowest_desc(descriptors, i, data->registers[A1]);
        }
    }

    // Read only and file does not exist
    if (!exists && data->registers[A1] == 0) {
        data->registers[V0] = -1;
    }
    // Write to a new file 
    if (!exists && data->registers[A1] == 1) {
        int i = 0;
        while (files[i].path != NULL && i < MAX_FILE_NUM) {
            i++;
        }
        files[i].path = strdup(path_name);
        data->registers[V0] = lowest_desc(descriptors, i, data->registers[A1]);
    }
}

/**
 * Finds and returns the lowest available file descriptor.
 */
static uint32_t lowest_desc(struct descriptor *descriptors, int i, 
                            uint32_t type) {
    int j = 0;
    // Find lowest unused file descriptor
    while (descriptors[j].file_index != -1 && j < MAX_DESC_NUM) {
        j++;
    }
    // Assign the file index
    descriptors[j].file_index = i;

    if (type == 0) {
        descriptors[j].read = true;
    } else {
        descriptors[j].write = true;
    }
    return j;
}

/**
 * Reads from a given file descriptor into a buffer address.
 */
static void read_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable) {
    int buffer_index = data->registers[A1] - MEMORY_START;
    int num_bytes = data->registers[A2];
    uint32_t desc_index = data->registers[A0];

    // Check if read is allowed.
    if (descriptors[desc_index].read == false) {
        data->registers[V0] = -1;

    } else {
        // Deals with reading beyond end of file data.
        int pos = descriptors[desc_index].pos;
        int read_size = 0;
        int file_size = files[descriptors[desc_index].file_index].size;
        if (pos + num_bytes > file_size) {
            read_size = file_size - pos;
        } else {
            read_size = num_bytes;
        }
        // Read contents
        int address = data->registers[5];
        for (int i = 0; i < read_size; i++) {
            address_check(address + i, executable, BYTE_LEN);
            executable->initial_data[buffer_index + i] = 
                files[descriptors[desc_index].file_index].data[pos + i];
        }
        data->registers[V0] = read_size;
        descriptors[desc_index].pos += read_size;
    }
}

/**
 * Writes to a given file descriptor with the contents of a given buffer 
 * address.
 */
static void write_file(struct runtime_data *data, struct file *files,
                       struct descriptor *descriptors, 
                       struct imps_file *executable) {
    int buffer_index = data->registers[A1] - MEMORY_START;
    int num_bytes = data->registers[A2];
    uint32_t desc_index = data->registers[A0];

    if (descriptors[data->registers[A0]].write == false) {
        data->registers[V0] = -1;
    } else {
        int pos = descriptors[desc_index].pos;
        int write_size = 0;
        if (pos + num_bytes > MAX_FILE_SIZE) {
            write_size = MAX_FILE_SIZE - pos;
        } else {
            write_size = num_bytes;
        }
        
        // Write to the file
        int address = data->registers[A1];
        for (int i = 0; i < write_size; i++) {
            address_check(address + i, executable, BYTE_LEN);
            files[descriptors[desc_index].file_index].data[pos + i] = 
                executable->initial_data[buffer_index + i];
        }
        // Determine new size of the file 
        int file_size = files[descriptors[desc_index].file_index].size;
        if (pos + write_size > file_size) {
            files[descriptors[desc_index].file_index].size = pos + write_size;
        }
        descriptors[desc_index].pos += write_size;
        data->registers[V0] = write_size;
    }
}

/**
 * Closes a file descriptor.
 */
static void close_file(struct runtime_data *data, struct file *files,
                      struct descriptor *descriptors, 
                      struct imps_file *executable) {
    uint32_t desc_index = data->registers[A0];
    // Check for valid descriptor
    if (desc_index < 0 || desc_index >= MAX_DESC_NUM) {
        data->registers[V0] = -1;
    } else if (descriptors[desc_index].file_index == -1) {
        data->registers[V0] = -1;
    } else {
        // Reset the descriptor's contents
        descriptors[desc_index].file_index = -1;
        descriptors[desc_index].pos = 0;
        descriptors[desc_index].read = false;
        descriptors[desc_index].write = false;
        data->registers[V0] = 0;
    }
}

/**
 * Adds the source and target register, storing the resulting value in the 
 * destination register. Performs an overflow check.
 */
static void add_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;
    uint32_t *registers = data->registers;
    if (destination != ZERO_REGISTER) {
        overflow_check(registers[target], registers[source]); 
        registers[destination] = registers[source] + registers[target];
    }   
    data->index++; 
}

/**
 * Counts the number of leading one's in the source register, storing the 
 * count in the destination register.
 */
static void clo_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;
    uint32_t *registers = data->registers;
    uint32_t count = 0;
    if (destination != 0) {
        // Check each bit 
        for (int i = 31; i >= 0; i--) {
            if (((registers[source] >> i) & 1) == 1) {
                count++;
            } else {
                break;
            }
        }
        registers[destination] = count;
    }
    data->index++;
}

/**
 * Counts the number of leading zero's in the source register, storing the 
 * count in the destination register.
 */
static void clz_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;
    uint32_t *registers = data->registers;
    uint32_t count = 0;
    if (destination != 0) {
        for (int i = 31; i >= 0; i--) {
            if (((registers[source] >> i) & 1) == 0) {
                count++;
            } else {
                break;
            }
        }
        registers[destination] = count;
    }
    data->index++;
}

/**
 * Adds the source and target register, storing the resulting value in the 
 * destination register.
 */
static void addu_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;
    uint32_t *registers = data->registers;
    if (destination != ZERO_REGISTER) {
        registers[destination] = registers[source] + registers[target];
   
    }   
    data->index++; 
}

/**
 * Compare the values in the source and target register. If the target register
 * is greater than the source register, 1 is stored in the destination register,
 * else 0.
 */
static void slt_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;
    uint32_t *registers = data->registers;
    if (destination != ZERO_REGISTER) {
        if ((int)registers[source] < (int)registers[target]) {
            registers[destination] = 1;
        } else {
            registers[destination] = 0;
        }
    }   
    data->index++; 
}

/**
 * If the given opcode and funct do not correspond to any implemented 
 * instruction, then an error is printed.
 */
static void print_bad_instruction(uint32_t execute, struct runtime_data *data,
                                  struct file *files,
                                  struct descriptor *descriptors) {
    fprintf(stderr, "IMPS error: bad instruction ");
    print_uint32_in_hexadecimal(stderr, execute);
    fprintf(stderr, "\n");
    free_data(data, files, descriptors);
    exit(EXIT_FAILURE); 
}

/**
 * Performs the bitwise operation OR between the source and immediate values,
 * storing the result in the target register.
 */
static void ori_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint16_t immediate = execute & IMMEDIATE_MASK;
    uint32_t *registers = data->registers;
    if (target != ZERO_REGISTER) {
        registers[target] = registers[source] | immediate;
    }
    data->index++;   
}

/**
 * Loads the immediate value into the top 16 bits of the target register.
 */
static void lui_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t immediate = execute & IMMEDIATE_MASK;
    if ((immediate >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        immediate -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    if (target != ZERO_REGISTER) {
        registers[target] = immediate << LUI_SHIFT;
    }
    data->index++;
}

/**
 * Stores the sum value of a register and an immediate value in the target 
 * register. 
 */
static void addiu_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t immediate = execute & IMMEDIATE_MASK;
    if ((immediate >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        immediate -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    if (target != ZERO_REGISTER) {
        registers[target] = registers[source] + immediate;
    }   
    data->index++;
}

/**
 * Stores the product of the source and target register in the destination
 * register.
 */
static void mul_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint8_t destination = (execute >> DESTINATION_SHIFT) & REGISTER_MASK;

    uint32_t *registers = data->registers;
    if (destination != ZERO_REGISTER) {
        registers[destination] = registers[source] * registers[target];
    }   
    data->index++; 
}

/**
 * Checks if the value in two registers is equal, if so, jumps to the next
 * instruction with a given offset.
 */
static void beq_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    if (registers[source] == registers[target]) {
        data->index += offset;
    } else {
        data->index++;
    }
}

/**
 * Checks if the value in two registers are not equal, if so, jumps to the next
 * instruction with a given offset.
 */
static void bne_inst(uint32_t execute, struct runtime_data *data) {
    uint8_t source = (execute >> SOURCE_SHIFT) & REGISTER_MASK;
    uint8_t target = (execute >> TARGET_SHIFT) & REGISTER_MASK;
    uint32_t offset = execute & OFFSET_MASK;
    if ((offset >> SIGN_BIT_SHIFT) & SIGN_BIT_MASK) {
        offset -= SIGN_BIT_EXTENSION;
    }
    uint32_t *registers = data->registers;
    if (registers[source] != registers[target]) {
        data->index += offset;
    } else {
        data->index++;
    }
}

/**
 * Function that checks the opcode and executes all memory related instructions.
 * If the opcode is not valid, then an error is printed.
 */
static void mem_inst(uint32_t execute, struct runtime_data *data,
                     struct imps_file *executable, struct file *files,
                     struct descriptor *descriptors) {
    uint8_t opcode = (execute >> OPCODE_SHIFT) & OPCODE_MASK;

    if (opcode == LB_INST) {
        lb_inst(execute, data, executable);
    } else if (opcode == LH_INST) {
        lh_inst(execute, data, executable);
    } else if (opcode == LW_INST) {
        lw_inst(execute, data, executable);
    } else if (opcode == SB_INST) {
        sb_inst(execute, data, executable);
    } else if (opcode == SH_INST) {
        sh_inst(execute, data, executable);
    } else if (opcode == SW_INST) {
        sw_inst(execute, data, executable);
    } else {
        print_bad_instruction(execute, data, files, descriptors);
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
