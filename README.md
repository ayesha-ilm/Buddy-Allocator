# Buddy Allocator

## Introduction

This project implements a memory management system using the **Buddy Memory Allocation** technique. The allocator provides dynamic memory allocation and deallocation in powers of two, which helps in efficiently managing memory with minimal fragmentation.

## Features

- Memory allocation in powers of two.
- Buddy system for efficient coalescing and splitting of memory blocks.
- Dynamic memory deallocation and coalescing of adjacent free blocks.
- Debugging functionality to display the state of free blocks.

## Files

- `my_allocator.c` - Implementation of the buddy allocator.
- `my_allocator.h` - Header file containing function declarations and structure definitions.
- `Makefile` - For building and compiling the project (optional).

## Functions

### `unsigned int init_allocator(unsigned int _basic_block_size, unsigned int _length)`
Initializes the memory allocator. This function allocates a block of memory and sets up the free list structure for various block sizes.

- `_basic_block_size`: The smallest block size.
- `_length`: The total size of memory to manage.

### `Addr my_malloc(unsigned int _length)`
Allocates a block of memory for the requested size `_length`.

- Returns the address of the allocated block or `NULL` if allocation fails.

### `int my_free(Addr _a)`
Frees the memory block at address `_a` and attempts to coalesce it with its buddy.

### `void print()`
Prints the state of the free lists, showing available blocks for each block size.

## Usage

1. **Initialize Allocator**:
   ```c
   init_allocator(4, 64);

2.  **Allocate Memory**:
```c
Addr ptr = my_malloc(16);

3. **Free Memory**:
```c
my_free(ptr);

4. **View Free Lists**:
```c
print();

## Example

int main() {
    init_allocator(4, 64);   // Initialize allocator with basic block size 4 and total memory 64

    Addr ptr1 = my_malloc(8);  // Allocate 8 bytes
    Addr ptr2 = my_malloc(16); // Allocate 16 bytes
    Addr ptr3 = my_malloc(32); // Allocate 32 bytes

    print();  // Print the current state of free blocks

    my_free(ptr1); // Free 8-byte block
    my_free(ptr2); // Free 16-byte block
    my_free(ptr3); // Free 32-byte block

    print();  // Print the free lists after deallocation
}

## Compiling and Running

To compile the program, use the following command:

gcc my_allocator.c -o my_allocator

To run the program:

./my_allocator

## Debugging

- Use printf statements inside functions like my_malloc, my_free, add_block, remove_block, and coalesce to monitor how memory blocks are being managed.
- The print function can be used to display the current state of the free lists.
