#include "SINVM.h"


// TODO: move error catching in syntax to the Assembler class; it doesn't really belong here


const bool SINVM::address_is_valid(size_t address) {
	// checks to see if the address we want to use is within 0 and memory_size

	return ((address >= 0) && (address < memory_size));
}


uint16_t SINVM::get_data_of_wordsize() {
	/*
	
	Since we might have a variable wordsize, we need to be able to handle numbers of various word sizes. As such, have a function to do this for us (since we will be doing it a lot).
	The routine is simple: initialize a variable to 0, read in the first byte, add it to our variable, and shift the bits left by 8; then, increment the PC. Continue until we have one more byte left to read, at which point, stop looping. Then, dereference the PC and add it to our variable. We can then return the variable.
	It is done this way so that it
		a) doesn't shift the bits too far over, requiring us to shift them back after
		b) doesn't increment the PC one too many times, requiring us to decrement it after

	Note on usage:
		The function should be executed *with the PC pointing to the first byte we want to read*. It will end with the PC pointing to the next byte to read after we are done.
	
	*/

	// load the appropriate number of bytes according to our wordsize
	uint16_t data_to_load = 0;

	// loop one time too few
	for (size_t i = 1; i < this->_WORDSIZE / 8; i++) {
		data_to_load = data_to_load + this->memory[this->PC];
		data_to_load = data_to_load << 8;
		this->PC++;
	}
	// and put the data in on the last time -- this is to prevent incrementing PC too much or bitshifting too far
	data_to_load = data_to_load + this->memory[this->PC];

	return data_to_load;
}

std::vector<uint8_t> SINVM::get_properly_ordered_bytes(uint16_t value) {
	std::vector<uint8_t> ordered_bytes;
	for (uint8_t i = (this->_WORDSIZE / 8); i > 0; i--) {
		ordered_bytes.push_back(value >> ((i - 1) * 8));
	}
	return ordered_bytes;
}


void SINVM::execute_instruction(uint16_t opcode) {
	/* 
	
	Execute a single instruction. Each instruction will increment or set the PC according to what it needs to do for that particular instruction. This function delegates the task of handling instruction execution to many other functions to make the code more maintainable and easier to understand.

	*/

	// use a switch statement; compiler may be able to optimize it more easily and it will be easier to read
	switch (opcode) {
		case HALT:
			// if we get a HALT command, we want to set the H flag, which will stop the VM in its main loop
			this->set_status_flag('H');
			break;
		case NOOP:
			// do nothing
			break;

		// load/store registers
		// all cases use the execute_load() and execute_store() functions, just on different registers
		case LOADA:
			REG_A = this->execute_load();
			break;
		case STOREA:
			this->execute_store(REG_A);
			break;

		case LOADB:
			REG_B = this->execute_load();
			break;
		case STOREB:
			this->execute_store(REG_B);
			break;

		case LOADX:
			REG_X = this->execute_load();
			break;
		case STOREX:
			this->execute_store(REG_X);
			break;
	
		case LOADY:
			REG_Y = this->execute_load();
			break;
		case STOREY:
			this->execute_store(REG_Y);
			break;

		// carry flag
		case CLC:
			this->clear_status_flag('C');
			break;
		case SEC:
			this->set_status_flag('C');
			break;
		case CLN:
			this->clear_status_flag('N');
			break;
		case SEN:
			this->set_status_flag('N');
			break;

		// ALU instructions
		// For these, we will use our load function to get the right operand, which will be the function parameter for the ALU functions
		case ADDCA:
		{
			// get the addend
			uint16_t addend = this->execute_load();

			// call the alu.add(...) function using the value we just fetched
			this->alu.add(addend);
			break;
		}
		case SUBCA:
		{
			// in subtraction, REG_A is the minuend and the value supplied is the subtrahend
			uint16_t subtrahend = this->execute_load();

			// call the alu.sub function using the value we just fetched
			this->alu.sub(subtrahend);
			break;
		}
		case MULTA:
		{
			// Multiply A by some value; treat both integers as signed
			uint16_t multiplier = this->execute_load();

			// call alu.mult_signed using the multiplier value we just fetched
			this->alu.mult_signed(multiplier);
			break;
		}
		case DIVA:
		{
			// Signed division on A by some value; this uses _integer division_ where B will hold the remainder of the operation
			// fetch the right operand and call the ALU div_signed function using said operand as an argument
			uint16_t divisor = this->execute_load();
			this->alu.div_signed(divisor);
			break;
		}
		case MULTUA:
		{
			// Unsigned multiplication
			// fetch the value and call ALU::mult_unsigned(...) using the value we fetched as our argument
			uint16_t multiplier = this->execute_load();
			this->alu.mult_unsigned(multiplier);
			break;
		}
		case DIVUA:
		{
			// Unsigned division; B will hold the remainder from the operation

			// fetch the right operand
			uint16_t divisor = this->execute_load();

			// call the ALU's div_unsigned function using the value we just fetched as the parameter
			this->alu.div_unsigned(divisor);
			break;
		}
		// todo: update logical operations to use the ALU
		case ANDA:
		{
			uint16_t and_value = this->execute_load();

			REG_A = REG_A & and_value;
			break;
		}
		case ORA:
		{
			uint16_t or_value = this->execute_load();

			REG_A = REG_A | or_value;
			break;
		}
		case XORA:
		{
			uint16_t xor_value = this->execute_load();

			REG_A = REG_A ^ xor_value;
			break;
		}
		case LSR: case LSL: case ROR: case ROL:
		{
			this->execute_bitshift(opcode);
			break;
		}

		// Incrementing / decrementing registers
		case INCA:
			this->REG_A = this->REG_A + 1;
			break;
		case DECA:
			this->REG_A -= 1;
			break;
		case INCX:
			this->REG_X += 1;
			break;
		case DECX:
			this->REG_X -= 1;
			break;
		case INCY:
			this->REG_Y += 1;
			break;
		case DECY:
			this->REG_Y -= 1;
			break;
		case INCB:
			this->REG_B += 1;
			break;
		// Note that INCSP and DECSP modify by one /word/, not one byte (it is unlike the other inc/dec instructions in this way)
		case INCSP:
			// increment by one word
			if (this->SP < (uint16_t)_STACK) {
				this->SP += (this->_WORDSIZE / 8);
			}
			else {
				throw VMException("Stack underflow.", this->PC);
			}
			break;
		case DECSP:
			// decrement by one word
			if (this->SP > (uint16_t)_STACK_BOTTOM) {
				this->SP -= (this->_WORDSIZE / 8);
			}
			else {
				throw VMException("Stack overflow.", this->PC);
			}
			break;

		// Comparatives
		case CMPA:
			this->execute_comparison(REG_A);
			break;
		case CMPB:
			this->execute_comparison(REG_B);
			break;
		case CMPX:
			this->execute_comparison(REG_X);
			break;
		case CMPY:
			this->execute_comparison(REG_Y);
			break;

		// Branch and control flow logic
		case JMP:
			this->execute_jmp();
			break;
		case BRNE:
			// if the comparison was unequal, the Z flag will be clear
			if (!this->is_flag_set('Z')) {
				// if it's clear, execute a jump
				this->execute_jmp();
			}
			else {
				// skip past the addressing mode and address if it isn't set
				// we need to skip past 3 bytes
				this->PC += 3;
			}
			break;
		case BREQ:
			// if the comparison was equal, the Z flag will be set
			if (this->is_flag_set('Z')) {
				// if it's set, execute a jump
				this->execute_jmp();
			}
			else {
				// skip past the addressing mode and address if it isn't set
				this->PC += 3;
			}
			break;
		case BRGT:
			// the carry flag will be set if the value is greater than what we compared it to
			if (this->is_flag_set('C')) {
				this->execute_jmp();
			}
			else {
				// skip past data we don't need
				this->PC += 3;
			}
			break;
		case BRLT:
			// the carry flag will be clear if the value is less than what we compared it to
			if (!this->is_flag_set('C')) {
				this->execute_jmp();
			}
			else {
				this->PC += 3;
			}
			break;
		case BRZ:
			// branch on zero; if the Z flag is set, branch; else, continue

			// TODO: delete this? do we really need it? it's equal to branch if equal...

			if (this->is_flag_set('Z')) {
				this->execute_jmp();
			}
			else {
				this->PC += 3;
			}
			break;
		case JSR:
		{
			// get the addressing mode
			this->PC++;
			uint8_t addressing_mode = this->memory[this->PC];

			// get the value
			this->PC++;
			uint16_t address_to_jump = this->get_data_of_wordsize();

			uint16_t return_address = this->PC;

			// if the CALL_SP is greater than _CALL_STACK_BOTTOM, we have not written past the end of it
			if (this->CALL_SP >= _CALL_STACK_BOTTOM) {
				// memory size is 16 bits but...do this anyways
				for (size_t i = 0; i < (this->_WORDSIZE / 8);  i++) {
					uint8_t memory_byte = return_address >> (i * 8);
					this->memory[CALL_SP] = memory_byte;
					this->CALL_SP--;
				}
			}
			else {
				throw VMException("Stack overflow on call stack.", this->PC);
			}

			this->PC = address_to_jump - 1;

			break;
		}
		case RTS:
		{
			uint16_t return_address = 0;
			if (this->CALL_SP <= _CALL_STACK) {
				for (size_t i = (this->_WORDSIZE / 8); i > 0; i--) {
					CALL_SP++;
					return_address += memory[CALL_SP];
					return_address = return_address << ((i - 1) * 8);
				}
			}
			this->PC = return_address;	// we don't need to offset because the absolute address was pushed to the call stack
			break;
		}

		// Register transfers
		case TBA:
			REG_A = REG_B;
			break;
		case TXA:
			REG_A = REG_X;
			break;
		case TYA:
			REG_A = REG_Y;
			break;
		case TSPA:
			REG_A = (uint16_t)SP;	// SP holds the address to which the next element in the stack will go, and is incremented every time something is pushed, and decremented every time something is popped
			break;
		case TSTATUSA:
			REG_A = (uint16_t)STATUS;
			break;
		case TAB:
			REG_B = REG_A;
			break;
		case TAX:
			REG_X = REG_A;
			break;
		case TAY:
			REG_Y = REG_A;
			break;
		case TASP:
			SP = (uint16_t)REG_A;
			break;
		case TASTATUS:
			STATUS = (uint8_t)REG_A;
			break;

		// The stack
		case PHA:
			this->push_stack(REG_A);
			break;
		case PLA:
			REG_A = this->pop_stack();
			break;
		case PHB:
			this->push_stack(REG_B);
			break;
		case PLB:
			REG_B = this->pop_stack();
			break;

		// SYSCALL INSTRUCTION
		// Note the syscall instruction serves many purposes; see "Doc/syscall.txt" for more information
		case SYSCALL:
		{
			this->execute_syscall();	// call the execute_syscall function; this will handle everything for us
			break;
		}

		// if we encounter an unknown opcode
		default:
		{
			throw VMException("Unknown opcode; halting execution.", this->PC);
			break;
		}
	}

	return;
}


uint16_t SINVM::execute_load() {
	/*
	
	Execute a LOAD_ instruction. This function takes a pointer to the register we want to load and executes the load instruction accordingly, storing the ultimate fetched result in the register we passed into the function.

	The function first reads the addressing mode, uses get_data_of_wordsize() to get the data after the addressing mode data (could be interpreted as an immediate value or memory address). It then checks the addressing mode to see how it needs to interpret the data it just fetched (data_to_load), and acts accordingly (e.g., if it's absolute, it gets the value at that memory address, then stores it in the register; if it is indexed, it does it almost the same, but adds the address value first, etc.).

	This function also makes use of 'validate_address()' to ensure all the addresses are within range.
	
	*/


	// the next data we have is the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	if (addressing_mode == addressingmode::reg_b) {
		return REG_B;
	}

	this->PC++;

	// load the appropriate number of bytes according to our wordsize
	uint16_t data_to_load = this->get_data_of_wordsize();

	/*
	
	If we are using short addressing, we will set a flag and subtract 0x10 (base for short addressing) from the addressing mode before proceeding. We will then pass our flag into the SINVM::get_data_from_memory(...) function; said function will operate appropriately for the short addressing mode

	*/

	bool is_short;
	if (addressing_mode >= addressingmode::absolute_short) {
		is_short = true;
		addressing_mode -= addressingmode::absolute_short;
	}
	else {
		is_short = false;
	}

	// check our addressing mode and decide how to interpret our data
	if ((addressing_mode == addressingmode::absolute) || (addressing_mode == addressingmode::x_index) || (addressing_mode == addressingmode::y_index)) {
		// if we have absolute or x/y indexed-addressing, we will be reading from memory
		uint16_t data_in_memory = 0;

		// however, we need to make sure that if it is indexed, we add the register values to data_to_load, which contains the memory address, so we actually perform the indexing
		if (addressing_mode == addressingmode::x_index) {
			// x indexed -- add REG_X to data_to_load
			data_to_load += REG_X;
		}
		else if (addressing_mode == addressingmode::y_index) {
			// y indexed
			data_to_load += REG_Y;
		}

		// check for a valid memory address
		if (!address_is_valid(data_to_load)) {
			// don't throw an exception if the address is out of range; subtract the memory size from data_to_load if it is over the limit until it is within range
			while (data_to_load > memory_size) {
				data_to_load -= memory_size;
			}
		}

		// now, data_to_load definitely contains the correct start address
		data_in_memory = this->get_data_from_memory(data_to_load, is_short);

		// load our target register with the data we fetched
		return data_in_memory;
	}
	else if (addressing_mode == addressingmode::immediate) {
		// if we are using immediate addressing, data_to_load is the data we want to load
		// simply assign it to our target register and return
		return data_to_load;
	}

	/*

	For these examples, let's say we have this memory map to work with:
			$0000	-	#$00
			$0000	-	#$00
			$0002	-	#$23
			$0003	-	#$C0
			...
			$23C0	-	#$AB
			$23C1	-	#$CD
			$23C2	-	#$12
			$23C3	-	#$34

	Indexed indirect modes go to some address, index it with X or Y, go to the address equal to that value, and fetch that. This acts as a pointer to a pointer, essentially. For example:
			loadx #$02
			loada ($00, x)
	will cause A to look at address $00, x, which contains the word $23C0; it then fetches the address at location $23C0, which is #$ABCD

	Our indirect indexed modes are like indexed indirect, but less insane; they get the value at the address and use that as a memory location to access, finally indexing with the X or Y registers. Essentially, this acts as a pointer. For example:
			loady #$02
			loada ($02), y
	will cause A to look at address $02, which contains the word $23C0; it then goes to that address and indexes it with the y register, so it points to address $23C2, which contains the word #$1234

	Both addressing modes can be used with the X and Y registers

	*/

	// indirect indexed
	else if (addressing_mode == addressingmode::indirect_indexed_x) {
		// indirect indexed addressing with the X register

		// get the data at the address indicated by data_to_load
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load);	// get the whole word (as it's an address), so don't use short addressing
		// now go to that address + reg_x and return the data there
		return this->get_data_from_memory(data_in_memory + REG_X, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
	else if (addressing_mode == addressingmode::indirect_indexed_y) {
		// indirect indexed addressing with the Y register
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory + REG_Y, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}

	// indexed indirect
	else if (addressing_mode == addressingmode::indexed_indirect_x) {
		// go to data_to_load + X, get the value there, go to that address, and return the value stored there
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load + REG_X);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
	else if (addressing_mode == addressingmode::indexed_indirect_y) {
		uint16_t data_in_memory = this->get_data_from_memory(data_to_load + REG_Y);	// get the whole word (as it's an address), so don't use short addressing
		return this->get_data_from_memory(data_in_memory, is_short);	// we _may_ want short addressing here, so pass the is_short flag
	}
}


void SINVM::execute_store(uint16_t reg_to_store) {
	// our next byte is the addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	// increment the program counter
	this->PC++;

	// next, get the memory location
	uint16_t memory_address = this->get_data_of_wordsize();

	/*
	
	Now, check to see if we are using short addressing. If so, subtract 0x10 (base for short addressing) from the addressing mode, set the is_short flag, and proceed. We will then pass is_short into the actual memory store function
	
	*/

	bool is_short;
	if (addressing_mode >= addressingmode::absolute_short) {
		is_short = true;
		addressing_mode -= addressingmode::absolute_short;
	}
	else {
		is_short = false;
	}

	// validate the memory location
	// TODO: decide whether writing to an invalid location will wrap around or simply throw an exception
	if (!address_is_valid(memory_address)) {
		// as long as it is out of range, continue subtracting the max memory size until it is valid
		while (memory_address > memory_size) {
			memory_address -= memory_size;
		}
	}

	// act according to the addressing mode
	if ((addressing_mode != addressingmode::immediate) && (addressing_mode != addressingmode::reg_a) && (addressing_mode != addressingmode::reg_b)) {
		// add the appropriate register if it is an indexed addressing mode
		if ((addressing_mode == addressingmode::x_index) || (addressing_mode == addressingmode::indirect_indexed_x)) {
			// since we will index after, we can include indirect indexed here -- if it's indirect, we simply get that value before we index
			if (addressing_mode == addressingmode::indirect_indexed_x) {
				memory_address = this->get_data_from_memory(memory_address);
			}
			// index the memory address with X
			memory_address += this->REG_X;
		}
		else if ((addressing_mode == addressingmode::y_index) || (addressing_mode == addressingmode::indirect_indexed_y)) {
			if (addressing_mode == addressingmode::indirect_indexed_y) {
				memory_address = this->get_data_from_memory(memory_address);
			}
			memory_address += this->REG_Y;
		}
		else if (addressing_mode == addressingmode::indexed_indirect_x) {
			memory_address = this->get_data_from_memory(memory_address + REG_X);
		}
		else if (addressing_mode == addressingmode::indexed_indirect_y) {
			memory_address = this->get_data_from_memory(memory_address + REG_Y);
		}

		// we are not allowed to store data in the word at location 0x00, as the word there is always guaranteed to be 0x00 (so that null pointers are always invalid)
		if (memory_address != 0x00 && memory_address != 0x01) {
			// use our store_in_memory function
			this->store_in_memory(memory_address, reg_to_store, is_short);
		}
		else {
			throw VMException("Write access violation; cannot write data to 0x00.", this->PC, this->STATUS);
		}

		return;
	}
	else if (addressing_mode == addressingmode::immediate) {
		// we cannot use immediate addressing with a store instruction, so throw an exception
		throw VMException("Invalid addressing mode for store instruction.", this->PC);
	}
}


uint16_t SINVM::get_data_from_memory(uint16_t address, bool is_short) {
	// read value of _WORDSIZE in memory and return it in the form of an int
	// different from "execute_load()" in that it doesn't affect the program counter and does not take addressing mode into consideration

	uint16_t data = 0;
	
	// if we are using the short addressing mode, get the individual byte
	if (is_short) {
		data = this->memory[address];
	}
	// otherwise, get the whole word
	else {
		size_t wordsize_bytes = this->_WORDSIZE / 8;

		size_t memory_index = 0;
		size_t bitshift_index = (wordsize_bytes - 1);

		while (memory_index < (wordsize_bytes - 1)) {
			data += this->memory[address + memory_index];
			data <<= (bitshift_index * 8);

			memory_index++;
			bitshift_index--;
		}
		data += this->memory[address + memory_index];
	}

	return data;
}

void SINVM::store_in_memory(uint16_t address, uint16_t new_value, bool is_short) {
	// stores value "new_value" in memory address starting at "high_byte_address"
	
	/*

	Make the assignment and return
	
	If we are using short addressing, we set the individual address to the new value and return

	Otherwise, if we are using the whole word:
	We can't do this quite the same way here as we did in the assembler; because our memory address needs to start low, and our bit shift needs to start high, we have to do a little trickery to get it right -- let's use 32-bit wordsize assigning to location $00 as an example:
	- First, we iterate from i=_WORDSIZE/8, so i=4
	- Now, we assign the value at address [memory_address + (wordsize_bytes - i)], so we assign to value memory_address, or $00
	- The value to assign is our value bitshifted by (i - 1) * 8; here, 24 bits; so, the high byte
	- On the next iteration, i = 3; so we assign to location [$00 + (4 - 3)], or $01, and we assign the value bitshifted by 16 bits
	- On the next iteration, i = 2; so we assign to location [$00 + (4 - 2)], or $02, and we assign the value bitshifted by 8 bits
	- On the final iteration, i = 1; so we assign to location [$00 + (4 - 1)], $03, and we assign the value bitshifted by (1 - 1); the value to assign is then reg_to_store >> 0, which is the low byte
	- That means we have the bytes ordered correctly in big-endian format

	*/

	if (is_short) {
		this->memory[address] = new_value;
	}
	else {
		size_t wordsize_bytes = this->_WORDSIZE / 8;
		for (size_t i = wordsize_bytes; i > 0; i--) {
			this->memory[address + (wordsize_bytes - i)] = new_value >> ((i - 1) * 8);
		}
	}

	return;
}

void SINVM::execute_bitshift(uint16_t opcode)
{

	// TODO: correct for wordsize -- currently, highest bit is always 7; should be highest bit in the wordsize

	// LOGICAL SHIFTS: 0 always shifted in; bit shifted out goes into carry
	// ROTATIONS: carry bit shifted in; bit shifted out goes into carry

	// check our addressing mode
	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	if (addressing_mode != addressingmode::reg_a) {
		uint16_t value_at_address;
		uint8_t high_byte_address;
		bool carry_set_before_bitshift = false;

		// increment PC so we can use get_data_of_wordsize()
		this->PC++;
		uint16_t address = this->get_data_of_wordsize();

		// if we have absolute addressing
		if (addressing_mode == addressingmode::absolute) {
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
				
				if (opcode == ROR && carry_set_before_bitshift) {
					// ORing value_at_address with 128 will always set bit 7
					value_at_address = value_at_address | 128;
				}
				else if (opcode == ROR && !carry_set_before_bitshift) {
					// ANDing with 127 will leave every bit unchanged EXCEPT for bit 7, which will be cleared
					value_at_address = value_at_address & 127;
				}
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;

				if (opcode == ROL && carry_set_before_bitshift) {
					// ORing value_at_address with 1 will always set bit 0
					value_at_address = value_at_address | 1;
				}
				else if (opcode == ROL && !carry_set_before_bitshift) {
					// ANDing with 254 will leave every bit unchanged EXCEPT for bit 0, which will be cleared
					value_at_address = value_at_address & 254;
				}
			}

			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have x-indexed addressing
		else if (addressing_mode == addressingmode::x_index) {
			address += REG_X;
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;
			}
			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have y-indexed addressing
		else if (addressing_mode == addressingmode::y_index) {
			address += REG_Y;
			high_byte_address = address >> 8;
			value_at_address = this->get_data_from_memory(high_byte_address);

			// get the original status of carry so we can set bits in ROR/ROL instructions later
			carry_set_before_bitshift = this->is_flag_set('C');

			// shift the bit
			if (opcode == LSR || opcode == ROR) {
				value_at_address = value_at_address >> 1;
			}
			else if (opcode == LSL || opcode == ROL) {
				value_at_address = value_at_address << 1;
			}

			this->store_in_memory(high_byte_address, value_at_address);
		}
		// if we have indirect addressing (x)
		else if (addressing_mode == addressingmode::indirect_indexed_x) {

			// TODO: enable indirect addressing

		}
		// if we have indirect addressing (y)
		else if (addressing_mode == addressingmode::indirect_indexed_y) {

			// TODO: enable indirect addressing

		}
		// if we have an invalid addressing mode
		// TODO: validate addressing mode in assembler for bitshifting instructions
		else {
			throw VMException("Cannot use that addressing mode with bitshifting instructions.", this->PC);
		}

		if (opcode == LSR || opcode == ROR) {
			// if bit 0 was set
			if ((value_at_address & 1) == 1) {
				// set the carry flag
				this->set_status_flag('C');
			}
			// otherwise
			else {
				// clear the carry flag
				this->clear_status_flag('C');
			}
		}
		else if (opcode == LSL || opcode == ROL) {
			if ((value_at_address & 128) == 128) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
		}
	}
	// if we have register addressing
	else {
		// so we know whether we need to set bit 7 or 0 after shifting
		bool carry_set_before_bitshift = this->is_flag_set('C');

		if (opcode == LSR || opcode == ROR) {
			// now, set carry according to the current bit 7 or 0
			if ((REG_A & 1) == 1) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
			// shift the bit right
			REG_A = REG_A >> 1;

			if (opcode == ROR && carry_set_before_bitshift) {
				// ORing with 128 will always set bit 7
				REG_A = REG_A | 128;
			}
			else if (opcode == ROR && !carry_set_before_bitshift) {
				// ANDing with 127 will always clear bit 7
				REG_A = REG_A & 127;
			}
		}
		else if (opcode == LSL || opcode == ROL) {
			// now, set carry according to the current bit 7 or 0
			if ((REG_A & 1) == 1) {
				this->set_status_flag('C');
			}
			else {
				this->clear_status_flag('C');
			}
			// shift the bit left
			REG_A = REG_A << 1;

			if (opcode == ROL) {
				// ORing with 1 will always set bit 1
				REG_A = REG_A | 1;
			}
			else if (opcode == ROL && !carry_set_before_bitshift) {
				// ANDing with 254 will always clear bit 1
				REG_A = REG_A & 254;
			}
		}
	}
}


void SINVM::execute_comparison(uint16_t reg_to_compare) {
	uint16_t to_compare = this->execute_load();

	// if the values are equal, set the Z flag; if they are not, clear it
	if (reg_to_compare == to_compare) {
		this->set_status_flag('Z');
	}
	else {
		this->clear_status_flag('Z');
		// we may need to set other flags too
		if (reg_to_compare < to_compare) {
			// set the carry flag if greater than, clear if less than
			this->clear_status_flag('C');
		}
		else {	// if it's not equal, and it's not less, it's greater
			this->set_status_flag('C');
		}
	}
	return;
}


void SINVM::execute_jmp() {
	/*

	Execute a JMP instruction. The VM first increments the PC to get the addressing mode, then increments it again to get the memory address

	*/

	this->PC++;
	uint8_t addressing_mode = this->memory[this->PC];

	// get the memory address to which we want to jump
	this->PC++;
	uint16_t memory_address = this->get_data_of_wordsize();

	// check our addressing mode to see how we need to handle the data we just received
	if (addressing_mode == addressingmode::absolute) {
		// if absolute, set it to memory address - 1 so when it gets incremented at the end of the instruction, it's at the proper address
		this->PC = memory_address - 1;
		return;
	}
	// indexed
	else if (addressing_mode == addressingmode::x_index) {
		memory_address += REG_X;
		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::y_index) {
		memory_address += REG_Y;
		this->PC = memory_address - 1;
	}
	// indexed indirect modes -- first, add the register value, get the value stored at that address, then use that fetched value as the memory location
	else if (addressing_mode == addressingmode::indexed_indirect_x) {
		size_t address_of_value = memory_address + REG_X;
		
		memory_address = this->get_data_from_memory(address_of_value);
		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::indexed_indirect_y) {
		size_t address_of_value = memory_address + REG_Y;

		memory_address = this->get_data_from_memory(address_of_value);
		this->PC = memory_address - 1;
	}
	// indirect indexed modes -- first, get the value stored at memory_address, then add the register index
	else if (addressing_mode == addressingmode::indirect_indexed_x) {
		/*
		
		First, we update the value of memory_address -- e.g., if memory_address was $1234:
			if $1234, $1235 had the value $23, $C0, we would get the value at $1234 -- $23C0
			then, we set memory_address to that value -- 23C0
			then, we add the value at the X or Y register
		
		*/
		
		memory_address = this->get_data_from_memory(memory_address);
		memory_address += REG_X;

		this->PC = memory_address - 1;
	}
	else if (addressing_mode == addressingmode::indirect_indexed_y) {
		memory_address = this->get_data_from_memory(memory_address);
		memory_address += REG_Y;

		this->PC = memory_address - 1;
	}
	else {
		throw VMException("Invalid addressing mode for JMP instruction.", this->PC);
	}

	return;
}


// Stack functions -- these always use register A, so no point in having parameters
void SINVM::push_stack(uint16_t reg_to_push) {
	// push the current value in "reg_to_push" (A or B) onto the stack, decrementing the SP (because the stack grows downwards)

	// first, make sure the stack hasn't hit its bottom
	if (this->SP < _STACK_BOTTOM) {
		throw VMException("Stack overflow.", this->PC);
	}

	for (size_t i = (this->_WORDSIZE / 8); i > 0; i--) {
		uint8_t val = (reg_to_push >> ((i - 1) * 8)) & 0xFF;
		this->memory[SP] = val;
		this->SP--;
	}

	return;
}

uint16_t SINVM::pop_stack() {
	// pop the most recently pushed value off the stack; this means we must increment the SP (as the current value pointed to by SP is the one to which we will write next; also, the stack grows downwards so we want to increase the address if we pop something off), and then dereference; this means this area of memory is the next to be written to if we push something onto the stack

	// first, make sure we aren't going beyond the bounds of the stack
	if (this->SP > _STACK) {
		throw VMException("Stack underflow.", this->PC);
	}

	// for each byte in a word, we must increment the stack pointer by one byte (increments BEFORE reading, as the SP points to the next AVAILABLE byte), get the data, shifted over according to what byte number we are on--remember we are reading the data back in little-endian byte order, not big, so we shift BEFORE we add
	uint16_t stack_data = 0;
	for (size_t i = 0; i < (this->_WORDSIZE / 8); i++) {
		this->SP++;
		stack_data += (this->memory[SP] << (i * 8));
	}

	// finally, return the stack data
	return stack_data;
}

void SINVM::free_heap_memory()
{
	/*
	
	Free the memory block starting at the memory address indicated by the B register. If there is no memory there, throw a VMException

	*/

	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	bool found = false;

	while ((obj_iter != this->dynamic_objects.end()) && !found) {
		// if the address of the object is the address we want to free, break from the loop; obj_iter contains the reference
		if (obj_iter->get_start_address() == REG_B) {
			found = true;
		}
		// otherwise, increment the iterator
		else {
			obj_iter++;
		}
	}

	if (found) {
		this->dynamic_objects.remove(*obj_iter);
	}
	else {
		throw VMException("Cannot free memory at location specified.");
	}
}

void SINVM::allocate_heap_memory()
{
	/*
	
	Attempts to allocate some memory on the heap. It tries to allocate REG_A bytes, and will load REG_B with the address where the object is located. If it cannot find any space for the object, it will load REG_A and REG_B with 0x00.
	
	*/
	
	// first, check to see the next available address in the heap that is large enough for this object -- use an iterator
	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	DynamicObject previous(_HEAP_START, 0);
	uint16_t next_available_address = 0x00;

	// if we have no DynamicObjects, the first location is _HEAP_START
	if (this->dynamic_objects.size() == 0) {
		next_available_address = _HEAP_START;
	}

	bool found_space = false;	// if we found space for the object somewhere in the middle of the list, use this to terminate the loop; we will use list::insert to add a new heap object at the position of the iterator

	while (obj_iter != this->dynamic_objects.end() && !found_space) {
		// check to see if there's room between the end of the previous object (which is the start address + size) and the start of the next object
		if (REG_A <= (obj_iter->get_start_address() - (previous.get_start_address() + previous.get_size()))) {
			// if there is, update the start address
			next_available_address = previous.get_start_address() + previous.get_size();
			found_space = true;
		}
		else {
			// update 'previous' and increment the object iterator
			previous = *obj_iter;
			obj_iter++;
		}
	}
	// do one last check against the very last item and the end of the heap
	if (REG_A <= (_HEAP_MAX - previous.get_start_address() + previous.get_size())) {
		next_available_address = previous.get_start_address() + previous.get_size();
	}

	// if the next available address is within our heap space, we are ok
	if ((next_available_address >= _HEAP_START) || (next_available_address <= _HEAP_MAX)) {
		REG_B = next_available_address;	// set the B register to the available address
		this->dynamic_objects.insert(obj_iter, DynamicObject(REG_B, REG_A));	// instead of appending and sorting, insert the object in the list -- this will be less computationally expensive ( O(n) vs O(n log n) )
	}
	else {
		// if the memory allocation fails, return a NULL pointer
		REG_B = 0x00;
		REG_A = 0x00;
	}
}

void SINVM::reallocate_heap_memory(bool error_if_not_found)
{
	/*
	
	Attempts to reallocate the dynamic object at the location specified by REG_B with the number of bytes in REG_A.
	If there is room for the new size where the object is currently allocated, then it will leave it where it is and simply change the size in the VM. If not, it will try to find a new place. If it can't reallocate the memory, it will load REG_A and REG_B with 0x00.
	If the VM cannot find an object at the location specified, it will:
		- Load the registers with 0x00 if 'error_if_not_found' is true
		- Allocate a new heap object if 'error_if_not_found' is false

	*/
	
	// iterate through the dynamic objects, trying to find the object we are looking for
	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	std::list<DynamicObject>::iterator target_object;
	bool found = false;

	while (obj_iter != dynamic_objects.end() && !found) {
		// if REG_B is equal to the address of obj_iter, we have found our object
		if (obj_iter->get_start_address() == this->REG_B) {
			found = true;
			target_object = obj_iter;
		}
		else {
			obj_iter++;
		}
	}

	// if we can't find the dynamic object, load the registers with 0x00
	if (found) {
		uint16_t original_address = obj_iter->get_start_address();
		uint16_t old_size = obj_iter->get_size();

		// because we sort the list when we allocate a heap object, the next object (if there is one) will be next in the order
		obj_iter++;	// increment the iterator
		
		// if we have another object in the vector, see if there is space for the new length
		if (obj_iter != dynamic_objects.end()) {
			// get the space between the start address and the end of the current object
			uint16_t buffer_space = obj_iter->get_start_address() - (target_object->get_start_address() + target_object->get_size());

			// if the new size is greater than the target object's size, but less than the target object's size plus the buffer, update the size
			// if it's less than or equal to the target object's size, we don't need to do anything
			if ((this->REG_A > target_object->get_size()) && (this->REG_A < (target_object->get_size() + buffer_space))) {
				target_object->set_size(this->REG_A);	// update the size to the value in REG_A
			}
			// otherwise, if it overflows the buffer, reallocate it
			else if (this->REG_A > (target_object->get_size() + buffer_space)) {
				// try to allocate it normally
				this->allocate_heap_memory();

				// copy the data from the old space into the new one
				memcpy(&this->memory[this->REG_B], &this->memory[original_address], old_size);

				// finally, remove the target object from the vector
				this->dynamic_objects.erase(target_object);	// because target_object is an iterator, we can remove it
			}
		}
		else {
			// otherwise, as long as we won't overrun the heap, it can stay
			if ((target_object->get_start_address() + this->REG_A) <= _HEAP_MAX) {
				target_object->set_size(this->REG_A);	// all we have to do is update the size
			}
			else {
				this->REG_A = 0x00;	// there's no room, so load them with 0x00
				this->REG_B = 0x00;
			}
		}
	}
	else {
		// depending on our parameter, the SINVM will behave differently -- load registers with NULL vs allocating a new object
		if (error_if_not_found) {
			this->REG_A = 0x00;	// we can't find the object, so set the registers to 0x00
			this->REG_B = 0x00;
		}
		else {
			this->allocate_heap_memory();	// allocate heap memory for the object if we can't find it
		}
	}
}


// STATUS register operations
void SINVM::set_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register

	if (flag == 'N') {
		this->STATUS |= StatusConstants::negative;
	}
	else if (flag == 'V') {
		this->STATUS |= StatusConstants::overflow;
	}
	else if (flag == 'U') {
		this->STATUS |= StatusConstants::undefined;
	}
	else if (flag == 'H') {
		this->STATUS |= StatusConstants::halt;
	}
	else if (flag == 'I') {
		this->STATUS |= StatusConstants::interrupt;
	}
	else if (flag == 'F') {
		this->STATUS |= StatusConstants::floating_point;
	}
	else if (flag == 'Z') {
		this->STATUS |= StatusConstants::zero;
	}
	else if (flag == 'C') {
		this->STATUS |= StatusConstants::carry;
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection in");	// throw an exception if we call the function with an invalid flag
	}

	return;
}

void SINVM::clear_status_flag(char flag) {
	// sets the flag equal to 'flag' in the status register

	if (flag == 'N') {
		this->STATUS = this->STATUS & (255 - StatusConstants::negative);
	}
	else if (flag == 'V') {
		this->STATUS = this->STATUS & (255 - StatusConstants::overflow);
	}
	else if (flag == 'U') {
		this->STATUS = this->STATUS & (255 - StatusConstants::undefined);
	}
	else if (flag == 'H') {
		this->STATUS = this->STATUS & (255 - StatusConstants::halt);
	}
	else if (flag == 'I') {
		this->STATUS = this->STATUS & (255 - StatusConstants::interrupt);
	}
	else if (flag == 'F') {
		this->STATUS = this->STATUS & (255 - StatusConstants::floating_point);
	}
	else if (flag == 'Z') {
		this->STATUS = this->STATUS & (255 - StatusConstants::zero);
	}
	else if (flag == 'C') {
		this->STATUS = this->STATUS & (255 - StatusConstants::carry);
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection");	// throw an exception if we call the function with an invalid flag
	}

	return;
}

uint8_t SINVM::get_processor_status() {
	return this->STATUS;
}

bool SINVM::is_flag_set(char flag) {
	// TODO: add new flags

	// return the status of a single flag in the status register
	// do this by performing an AND operation on that bit; if set, we will get true (the only bit that can be set is the bit in question)
	if (flag == 'N') {
		return this->STATUS & StatusConstants::negative; // will only return true if N is set; if any other bit is set, it will be AND'd against a 0
	}
	else if (flag == 'V') {
		return this->STATUS & StatusConstants::overflow;
	}
	else if (flag == 'U') {
		return this->STATUS & StatusConstants::undefined;
	}
	else if (flag == 'H') {
		return this->STATUS & StatusConstants::halt;
	}
	else if (flag == 'I') {
		return this->STATUS & StatusConstants::interrupt;
	}
	else if (flag == 'F') {
		return this->STATUS & StatusConstants::floating_point;
	}
	else if (flag == 'Z') {
		return this->STATUS & StatusConstants::zero;
	}
	else if (flag == 'C') {
		return this->STATUS & StatusConstants::carry;
	}
	else {
		throw std::runtime_error("Invalid STATUS flag selection");	// throw an exception if we call the function with an invalid flag
	}
}


void SINVM::run_program() {
	// as long as the H flag (HALT) is not set (the VM has not received the HALT signal)
	while (!(this->is_flag_set('H'))) {
		// execute the instruction pointed to by the program counter
		this->execute_instruction(this->memory[this->PC]);

		// advance the program counter to point to the next instruction
		this->PC++;
	}

	return;
}



void SINVM::_debug_values() {
	std::cout << "SINVM Values:" << std::endl;
	std::cout << "\t" << "Registers:" << "\n\t\tA: $" << std::hex << this->REG_A << std::endl;
	std::cout << "\t\tB: $" << std::hex << this->REG_B << std::endl;
	std::cout << "\t\tX: $" << std::hex << this->REG_X << std::endl;
	std::cout << "\t\tY: $" << std::hex << this->REG_Y << std::endl;
	std::cout << "\t\tSP: $" << std::hex << this->SP << std::endl;
	std::cout << "\t\tSTATUS: $" << std::hex << (uint16_t)this->STATUS << std::endl << std::endl;

	std::cout << "Memory: " << std::endl;
	for (size_t i = 0; i < 0xFF; i++) {
		// display the first two pages of memory
		std::cout << "\t$000" << i << ": $" << std::hex << (uint16_t)this->memory[i] << "\t\t$0" << 0x100 + i << ": $" << (uint16_t)this->memory[256 + i] << std::endl;
	}

	std::cout << "\nStack: " << std::endl;
	for (size_t i = 0xff; i > 0x00; i--) {
		// display the top page of the stack
		if (i > 0x0f) {
			std::cout << "\t$23" << i << ": $" << std::hex << (int)this->memory[0x2300 + i] << std::endl;
		}
		else {
			std::cout << "\t$230" << i << ": $" << std::hex << (int)this->memory[0x2300 + i] << std::endl;
		}
	}

	std::cout << std::endl;
}



SINVM::SINVM(std::istream& file)
{
	// TODO: redo the SINVM constructor...

	// initialize our ALU
	this->alu = ALU(&this->REG_A, &this->REG_B, &this->STATUS);

	// get the wordsize, make sure it is compatible with this VM
	uint8_t file_wordsize = BinaryIO::readU8(file);
	if (file_wordsize != _WORDSIZE) {
		throw VMException("Incompatible word sizes; the VM uses a " + std::to_string(_WORDSIZE) + "-bit wordsize; file to execute uses a " + std::to_string(file_wordsize) + "-bit word.");
	}

	size_t prg_size = (size_t)BinaryIO::readU32(file);
	std::vector<uint8_t> prg_data;

	for (size_t i = 0; i < prg_size; i++) {
		uint8_t next_byte = BinaryIO::readU8(file);
		prg_data.push_back(next_byte);
	}

	// copy our instructions into memory so that the program is at the end of memory
	this->program_start_address = _PRG_BOTTOM;	// the start address is $2600

	// if the size of the program is greater than 0xF000 - 0x2600, it's too big
	if (prg_data.size() > (_PRG_TOP - _PRG_BOTTOM)) {
		throw VMException("Program too large for conventional memory map!");

		// TODO: remap memory if the program is too large instead of exiting?

	}

	// copy the program data into memory
	std::vector<uint8_t>::iterator instruction_iter = prg_data.begin();
	size_t memory_index = this->program_start_address;
	while ((instruction_iter != prg_data.end())) {
		this->memory[memory_index] = *instruction_iter;
		memory_index++;
		instruction_iter++;
	}

	// initialize our list of dynamic objects as an empty list
	this->dynamic_objects = {};

	// initialize the stack to location to our stack's upper limit; it grows downwards
	this->SP = _STACK;
	this->CALL_SP = _CALL_STACK;

	// always initialize our status register so that all flags are not set
	this->STATUS = 0;

	// Set the word at 0x00 to 0 so that null pointers will not ever be valid
	this->memory[0] = 0;
	this->memory[1] = 0;

	// initialize the program counter to start at the top of the program
	if (prg_data.size() != 0) {
		this->PC = this->program_start_address;	// make sure that we don't read memory that doesn't exist if our program is empty
	}
	else {
		// throw an exception; the VM cannot execute an empty program
		throw VMException("Cannot execute an empty program; program size must be > 0");
	}
}

SINVM::~SINVM()
{
}
