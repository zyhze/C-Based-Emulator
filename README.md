# IMPS: A Simple MIPS Emulator

## Overview

This project involved the development of **IMPS**, a simple MIPS emulator written in C. The goal was to understand the encoding and semantics of MIPS instructions, practice file operations in C, and gain experience with UNIX file system syscalls. The emulator was designed to execute a subset of MIPS instructions and handle system calls, with additional features such as tracing mode and file system emulation.

## Project Structure

The project was divided into four main sections:

### Section 1: Reading IMPS Executable Files
- Implemented the `read_imps_file` function to read IMPS executable files into a structured format.
- The executable files contained sections such as the magic number, number of instructions, entry point, instructions, debug offsets, memory size, and initial data.
- Handled errors such as invalid magic numbers and unopenable files.

### Section 2: Basic Instruction Execution
- Implemented the `execute_imps` function to execute a subset of MIPS instructions, including `ADDI`, `ADD`, `ORI`, `LUI`, and `SYSCALL`.
- Managed system calls for printing integers, characters, and exiting the program.
- Handled errors such as invalid instructions, bad syscall numbers, and execution past the end of instructions.

### Section 3: Advanced Instructions and Memory Access
- Extended the emulator to support additional instructions such as `CLO`, `CLZ`, `ADDU`, `ADDIU`, `MUL`, `BEQ`, `BNE`, `SLT`, and memory access instructions (`LB`, `LH`, `LW`, `SB`, `SH`, `SW`).
- Implemented system calls for printing strings and reading characters.
- Added error handling for signed overflow in addition instructions and invalid memory accesses.

### Section 4: Tracing Mode and File System Syscalls
- Added a tracing mode that prints assembly source lines and register changes during execution.
- Implemented file system syscalls (`open`, `read`, `write`, `close`) with an in-memory emulated filesystem.
- Handled file operations such as reading, writing, and closing files, with error checking for invalid file descriptors and operations.

## Key Features

- **Tracing Mode:** Enabled by the `-t` flag, this mode printed the corresponding assembly source line before executing each instruction and displayed register changes after execution.
- **File System Emulation:** Implemented an in-memory filesystem to handle file operations without interacting with the actual file system.
- **Error Handling:** The emulator handled various errors, including invalid instructions, bad syscall numbers, memory access violations, and file operation errors.

## Testing and Validation

The project was tested using a series of automated tests provided by the `1521 autotest` command. The emulator was also compared against a reference implementation to ensure correct behavior. Additional testing was done using custom MIPS programs and edge cases to validate the emulator's functionality.

## Limitations and Improvements

- **Limitations:** The emulator only supported a subset of MIPS instructions and syscalls. The in-memory filesystem had a limited number of files and file size constraints.
- **Improvements:** Future work could include supporting more MIPS instructions, enhancing the filesystem to handle larger files, and improving error messages for better debugging.
