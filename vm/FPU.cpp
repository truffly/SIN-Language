/*

SIN Toolchain
FPU.cpp
Copyright 2019 Riley Lannon

Contains the implementation of the FPU class

*/

#include "FPU.h"

// todo: handle floating point errors that may be generated by C++, or implement floating point arithmetic manually (without uint32_t -> float conversion)

uint32_t FPU::combine_registers()
{
	/*
	
	Constructs a 32-bit value from two 16-bit values, REG_A and REG_B, where A contains the most significant bits and B contains the least
	
	*/

	uint32_t left = (*this->REG_A << 16) | *this->REG_B;
	return left;
}

void FPU::split_to_registers(uint32_t to_split)
{
	/*

	Splits a 32-bit value into two 16-bit halves, one in REG_A and the other in REG_B -- opposite of FPU::get_left()

	*/

	*this->REG_A = (to_split >> 16) & 0xFFFF;
	*this->REG_B = to_split & 0xFFFF;

	return;
}


/*

Half-precision instructions
These functions implement the half-precision floating-point operations, which actually use the 32-bit FPU operations.
The functions unpack the halfs as singles, pass them into the 32-bit functions, and pack them back into 16-bit format.

Since they call the 32-bit functions, the 16-bit ones will not affect the STATUS register, as that will be done by the 32-bit functions
However, the 16-bit functions may affect the STATUS register if the result was unexpected -- for overflow, etc., that may not be caught by 32-bit

todo: since these functions are all almost the same, can we combine their implementations in such a way that they don't need to take up so much space?

*/

void FPU::fadda(uint16_t right) {
	// half-precision addition
	uint32_t left_single = unpack_16(*this->REG_A);		// make this into a function returning a tuple?
	uint32_t right_single = unpack_16(right);

	// split the 32-bit left value in half
	this->split_to_registers(left_single);

	// perform the addition
	this->single_fadda(right_single);

	// re-pack the result
	uint32_t result = ((*this->REG_A << 16) & 0xFFFF0000) | ((*this->REG_B) & 0xFFFF);
	*this->REG_A = pack_32(result);

	// check for overflow

	return;
}

void FPU::fsuba(uint16_t right) {
	// half-precision subtraction
	uint32_t left_single = unpack_16(*this->REG_A);
	uint32_t right_single = unpack_16(right);

	this->split_to_registers(left_single);

	// perform the subtraction
	this->single_fsuba(right_single);

	// re-pack the result
	uint32_t result = this->combine_registers();
	*this->REG_A = pack_32(result);

	return;
}

void FPU::fmulta(uint16_t right) {
	// half-precision multiplication
	uint32_t left_single = unpack_16(*this->REG_A);
	uint32_t right_single = unpack_16(right);

	this->split_to_registers(left_single);

	this->single_fmulta(right_single);

	uint32_t result = this->combine_registers();
	*this->REG_A = pack_32(result);

	return;
}

void FPU::fdiva(uint16_t right) {
	// half-precision division
	uint32_t left_single = unpack_16(*this->REG_A);
	uint32_t right_single = unpack_16(right);

	this->split_to_registers(left_single);

	this->single_fdiva(right_single);

	uint32_t result = this->combine_registers();
	*this->REG_A = pack_32(result);

	return;
}


/*

Single-precision instructions
These functions implement single-precision floating-point operations, opereating on 32-bit values.
The 32-bit left operand is constructed by using the A register as the most significant 16 bits, and the B register as the least significant

*/

void FPU::single_fadda(uint32_t right)
{
	/*
	
	Adds two floating point numbers together, converting the uint32_t types to float first (using reinterpret_cast)

	todo: modify how STATUS register is affected -- C, V flags

	*/
	
	// get the left-hand value from REG_A and REG_B
	uint32_t left = this->combine_registers();
	
	// convert to float, perform operation
	float* left_f = reinterpret_cast<float*>(&left);
	float* right_f = reinterpret_cast<float*>(&right);
	*left_f += *right_f;

	if (*left_f == 0) {
		*this->STATUS |= StatusConstants::zero;
	}
	
	// since left_f was a pointer to left, the value in left was changed by our arithmetic; as such, we can split left to registers without casting
	this->split_to_registers(left);
	*this->STATUS |= StatusConstants::floating_point;	// set the floating-point flag

	return;
}

void FPU::single_fsuba(uint32_t right)
{
	// single-precision subtraction

	uint32_t left = this->combine_registers();	// get the left-hand value

	float* left_f = reinterpret_cast<float*>(&left);
	float* right_f = reinterpret_cast<float*>(&right);
	*left_f -= *right_f;

	if (*left_f == 0) {
		*this->STATUS |= StatusConstants::zero;
	}

	this->split_to_registers(left);
	*this->STATUS |= StatusConstants::floating_point;

	return;
}

void FPU::single_fmulta(uint32_t right)
{
	// single-precision multiplication

	uint32_t left = this->combine_registers();	// get the left-hand value

	float* left_f = reinterpret_cast<float*>(&left);
	float* right_f = reinterpret_cast<float*>(&right);

	*left_f *= *right_f;

	if (*left_f == 0) {
		*this->STATUS |= StatusConstants::zero;
	}

	this->split_to_registers(left);
	*this->STATUS |= StatusConstants::floating_point;

	return;
}

void FPU::single_fdiva(uint32_t right)
{
	// single-precision division

	uint32_t left = this->combine_registers();	// get the left-hand value

	float* left_f = reinterpret_cast<float*>(&left);
	float* right_f = reinterpret_cast<float*>(&right);
	
	if (*right_f == 0) {
		*this->STATUS |= StatusConstants::undefined;
		return;
	}
	else {
		if (*left_f == 0) {
			*this->STATUS |= StatusConstants::zero;
		}

		*left_f /= *right_f;

		this->split_to_registers(left);
		*this->STATUS |= StatusConstants::floating_point;

		return;
	}
}


/*

Constructor / Destructor

*/


FPU::FPU(uint16_t* REG_A, uint16_t* REG_B, uint16_t* STATUS): REG_A(REG_A), REG_B(REG_B), STATUS(STATUS) {
	
}

FPU::FPU() {
	// initialize the pointers to nullptr in the default constructor
	this->REG_A = nullptr;
	this->REG_B = nullptr;
	this->STATUS = nullptr;
}

FPU::~FPU() {

}
