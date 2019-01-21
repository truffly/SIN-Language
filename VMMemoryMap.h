#pragma once

/*

For SIN VM version 1

Contains the constants that define where our various blocks of memory in the VM begin

Our addresses range from $0000 to $FFFF (65k):
	- All variables will get stored from $0000 up to (but not including) $1400
	- An input buffer lives from $1400 to $17FF
	- The stack lives from $1800 to (but not including) $2400
	- The call stack lives from $2400 to (but not including) $2600
	- All included program data lives from $2600 to $f000
	- The arguments and command line data take up the last few pages -- $f000 to $ffff

Note that the stacks in the VM grow downward

*/

// declare how much memory the virtual machine has
const size_t memory_size = 0x10000;	// 16k available to the VM

// declare our start addresses for different sections of memory

// lower limit
const size_t _MEMORY_MIN = 0x0000;

// the data section gets addresses $0000 through $1FFF -- 32 pages
const size_t _HEAP = 0x0000;	// our global data section will always start at $0000
const size_t _BUFFER_START = 0x1400;	// a buffer for input data -- addresses from 1400 to 17ff will be untouched (meaning we can have 1,024 bytes of buffer by default)

// TODO: delete _LOCAL_DATA ?
const size_t _LOCAL_DATA = 0x1800;	// our local data section will start at $1800; this is the same as the stack bottom

// the RS directive, which creates global variables, will allocate variables starting at 0x0100 and have 3 pages; the zero page remains untouched by @rs and will be used for pointer tables
const size_t _RS_START = 0x0100;
// any variables allocated on the heap will be allocated starting at 0x0400 up to the stack; this is where all dynamic memory will be stored
const size_t _DYNAMIC_START = 0x0400;

// the stack -- this is used for all of our scope data, and so it gets quite a bit of memory
const size_t _STACK = 0x23FF;	// our stack grows downwards
const size_t _STACK_BOTTOM = 0x1800;	// the bottom of the stack

// call stack gets 2 pages
const size_t _CALL_STACK = 0x25FF;	// the call stack also grows downwards
const size_t _CALL_STACK_BOTTOM = 0x2400;

// program data itself
const size_t _PRG_TOP = 0xf000;	// the limit for our program data
const size_t _PRG_BOTTOM = 0x2600;	// our lowest possible memory address for the program

// program environment / command - line arguments
const size_t _ARG = 0xf000;	// f000 - ffff available for command-line/environment arguments

// upper limit
const size_t _MEMORY_MAX = 0xffff;