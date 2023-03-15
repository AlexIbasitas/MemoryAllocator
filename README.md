# MemoryAllocator

ABOUT

This is a memory allocator that allows a user to allocate memory, free memory, and peek into the current state of the heap. 

HOW TO USE

    Compile the program with the command 'gcc -g MemoryAllocator.c -o MemoryAllocator'
    Run the executable with './MemoryAllocator'
    You may optionally run './MemoryAllocator FirstFit' or './MemoryAllocator BestFit' which           specifies the algorithm in which free blocks will be allocated.

COMMANDS

    'malloc <size>' which allocates a block of memory where size is the amount of bytes for the         payload.
    'free <index>' which frees a block of memory at the start of the payload.
    'blocklist' which prints out all free/allocated blocks in memory in the format 'payload_size'-      'start_of_payload'-'allocation-status'.
    'writemem <index> <data> which writes characters into memory at the specified index.
    'printmem <index> <num_to_print> which prints segments of memory beginning at the specified         index.
    'quit' to quit the program
    



