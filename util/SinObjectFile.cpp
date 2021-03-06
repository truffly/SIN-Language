#include "SinObjectFile.h"

AssemblerData::AssemblerData(uint8_t _wordsize, std::vector<uint8_t> _text) : _wordsize(_wordsize), _text(_text)
{

}

// implement the AssemblerData type
AssemblerData::AssemblerData() {
	this->_data_table = {};
	this->_relocation_table = {};
	this->_symbol_table = {};
	this->_text = {};
	this->_wordsize = 0;
}

AssemblerData::~AssemblerData() {

}

// TODO: return the symbol table and relocation table as well...

void SinObjectFile::load_sinc_file(std::istream & file)
{
	// first, read the magic number
	char header[4];
	char * buffer = &header[0];
	file.read(buffer, 4);

	// if our magic number is valid
	if (std::strcmp(header, "sinC")) {

		// get the word size
		this->_wordsize = BinaryIO::readU8(file);

		// get the endianness
		uint8_t _text_end = BinaryIO::readU8(file);
		uint8_t _sinc_end = BinaryIO::readU8(file);

		// get the file version
		uint8_t _file_version = BinaryIO::readU8(file);	// to be used by the file reader -- currently not needed by linker
		this->_sinvm_version = BinaryIO::readU8(file);

		// get the entry point
		this->_text_start = BinaryIO::readU16(file);

		// TODO: validate the file using the file header data


		if (_file_version == 2) {
			
			// get the size of the program
			size_t _prog_size = (size_t)BinaryIO::readU32(file);


			// symbol data:

			// number of elements in symbol table
			size_t _st_size = (size_t)BinaryIO::readU32(file);

			// iterate for each item in _st_size
			for (size_t i = 0; i < _st_size; i++) {
				// get the value of the symbol
				uint16_t _sy_val = BinaryIO::readU16(file);
				uint8_t	_sy_width = BinaryIO::readU8(file);
				uint8_t _class = BinaryIO::readU8(file);
				std::string _sy_name = BinaryIO::readString(file);	// _sn_len is automatically retrieved from BianryIO::BinaryIO::readString

				// get the symbol class as a string from the uint8_t we read in
				SymbolClass symbol_class;
				if (_class == 1) {
					symbol_class = U;
				}
				else if (_class == 2) {
					symbol_class = D;
				}
				else if (_class == 3) {
					symbol_class = C;
				}
				else if (_class == 4) {
					symbol_class = R;
				}
				else if (_class == 5) {
					symbol_class = M;
				}
				else {
					throw std::runtime_error("**** Error: bad number in symbol class specifier.");
				}

				// create the tuple and push it onto "symbol_table"
				this->symbol_table.push_back(AssemblerSymbol(_sy_name, _sy_val, _sy_width, symbol_class));
			}


			// relocation data:

			// number of elements in relocation table
			size_t _rt_size = (size_t)BinaryIO::readU32(file);

			// iterate for each item in _rt_size
			for (size_t i = 0; i < _rt_size; i++) {
				// get the address where the symbol occurs in the code
				int _addr = (int)BinaryIO::readU16(file);
				std::string _sy_name = BinaryIO::readString(file);	// _rs_len is automatically retrieved by BinaryIO::BinaryIO::readString

				// create the tuple and push it onto "relocation_table"
				this->relocation_table.push_back(RelocationSymbol(_sy_name, _addr));
			}


			// .text:

			// iterate for each byte in _prog_size
			for (size_t i = 0; i < _prog_size; i++) {
				// read a byte from the file and push it onto program_data
				this->program_data.push_back(BinaryIO::readU8(file));
			}

			// .data:

			// read the number of constants to read
			size_t num_data_entries = (size_t)BinaryIO::readU32(file);

			// the data section immediately follows the program data
			size_t data_position_offset = this->program_data.size();

			for (size_t i = 0; i < num_data_entries; i++) {
				size_t num_bytes = BinaryIO::readU16(file);

				std::string macro_name = BinaryIO::readString(file);
				std::vector<uint8_t> data_bytes;

				for (size_t j = 0; j < num_bytes; j++) {
					data_bytes.push_back(BinaryIO::readU8(file));
				}

				// add the data to our data table
				this->data_table.push_back(std::make_tuple(macro_name, data_position_offset, data_bytes));

				// increment our data_position_offset by the number of data_bytes we have
				data_position_offset += data_bytes.size();
			}

			// .bss:
		}
		// cannot handle any other versions right now because they don't exist yet
		else {
			throw std::runtime_error("Other .sinc file versions not supported at this time.");
		}
	}
	else {
		throw std::runtime_error("Invalid magic number in file header.");
	}
}

void SinObjectFile::write_sinc_file(std::string output_file_name, AssemblerData assembler_obj)
{
	/*
	
	This routine generates a SIN object file (.sinc) using a given assembler object.
	
	*/

	// first, add the file name to our list of files we need linked
	// TODO: add name of file we need linked in the Assembler itself, not here
	//assembler_obj->obj_files_to_link.push_back(output_file_name + ".sinc");

	// create a binary file of the specified name (with the sinc extension)
	std::ofstream sinc_file(output_file_name + ".sinc", std::ios::out | std::ios::binary);

	// create a vector<int> to hold the binary program data; it will be initialized to our assembled file
	std::vector<uint8_t> program_data = assembler_obj._text;
	// after assembly, "assembler_obj->relocation_table" and "assembler_obj->symbol_table" will contain the correct data



	/************************************************************
	********************	FILE HEADER		*********************
	************************************************************/


	// Write the magic number to the file header
	const char * header = ("sinC");
	sinc_file.write(header, 4);

	// write the word size
	BinaryIO::writeU8(sinc_file, assembler_obj._wordsize);

	// write the endianness
	BinaryIO::writeU8(sinc_file, 2);	// sinVM uses big endian for its byte order
	BinaryIO::writeU8(sinc_file, 1);	// BinaryIO uses little endian

	// write the version information
	BinaryIO::writeU8(sinc_file, sinc_version);
	BinaryIO::writeU8(sinc_file, 1);	// target sinVM version is 1

	// entry point
	BinaryIO::writeU16(sinc_file, 0x00);



	/************************************************************
	********************	PROGRAM HEADER	*********************
	************************************************************/



	// write the size of the program
	BinaryIO::writeU32(sinc_file, program_data.size());


	// Symbol table info:

	// write the number of entries in our symbol table
	int num_symbols = assembler_obj._symbol_table.size();
	BinaryIO::writeU32(sinc_file, num_symbols);

	// write data in for each symbol in the table
	for (std::list<AssemblerSymbol>::iterator symbol_iter = assembler_obj._symbol_table.begin(); symbol_iter != assembler_obj._symbol_table.end(); symbol_iter++) {
		// get the various values
		std::string symbol_name = symbol_iter->name;
		uint8_t symbol_class;

		// set the symbol class
		if (symbol_iter->symbol_class == U) {
			symbol_class = 1;
		}
		else if (symbol_iter->symbol_class == D) {
			symbol_class = 2;
		}
		else if (symbol_iter->symbol_class == C) {
			symbol_class = 3;
		}
		else if (symbol_iter->symbol_class == R) {
			symbol_class = 4;
		}
		else if (symbol_iter->symbol_class == M) {
			symbol_class = 5;
		}
		else {
			throw std::runtime_error("Invalid symbol class specifier");
		}

		// write the symbol value and class
		BinaryIO::writeU16(sinc_file, (uint16_t)symbol_iter->value);
		BinaryIO::writeU8(sinc_file, (uint8_t)symbol_iter->width);
		BinaryIO::writeU8(sinc_file, symbol_class);

		// and use BinaryIO::writeString to write the symbol's name
		BinaryIO::writeString(sinc_file, symbol_name);
	}


	// Relocation table info:

	// write the number of entries in the relocation table
	int num_relocation_entries = assembler_obj._relocation_table.size();
	BinaryIO::writeU32(sinc_file, num_relocation_entries);

	// write the data for each symbol in the relocation table
	for (std::list<RelocationSymbol>::iterator relocation_iter = assembler_obj._relocation_table.begin(); relocation_iter != assembler_obj._relocation_table.end(); relocation_iter++) {
		// write the address it points to in the program
		BinaryIO::writeU16(sinc_file, relocation_iter->value);

		// write the name of the symbol
		BinaryIO::writeString(sinc_file, relocation_iter->name);
	}



	/************************************************************
	********************	.TEXT SECTION	*********************
	************************************************************/

	// write each byte of data in sequentually by using a vector iterator
	for (std::vector<uint8_t>::iterator data_iter = program_data.begin(); data_iter != program_data.end(); data_iter++) {
		// write the value to the file
		BinaryIO::writeU8(sinc_file, *data_iter);
	}



	/************************************************************
	********************	.DATA SECTION	*********************
	************************************************************/

	// Data header

	// write in the number of entries
	BinaryIO::writeU32(sinc_file, assembler_obj._data_table.size());

	// write in all of the constants
	// iterate through the data_table and write data accordingly
	for (std::list<DataSymbol>::iterator it = assembler_obj._data_table.begin(); it != assembler_obj._data_table.end(); it++) {
		// 0x00 - 0x01	->	number of bytes in the constant
		BinaryIO::writeU16(sinc_file, it->data.size());

		// symbol name
		BinaryIO::writeString(sinc_file, it->name);

		// the data
		for (std::vector<uint8_t>::iterator data_iter = it->data.begin(); data_iter != it->data.end(); data_iter++) {
			BinaryIO::writeU8(sinc_file, *data_iter);
		}
	}


	/************************************************************
	********************	.BSS SECTION	*********************
	************************************************************/

	// .BSS header

	// number of macros in .bss
	// TODO: fully implement rs directive
	// write in all of the non-constant macro names
	// TODO: complete .BSS


	sinc_file.close();
}



uint8_t SinObjectFile::get_wordsize() {
	return this->_wordsize;
}

std::vector<uint8_t> SinObjectFile::get_program_data() {
	return this->program_data;
}

std::list<AssemblerSymbol>* SinObjectFile::get_symbol_table()
{
	return &this->symbol_table;
}

std::list<std::tuple<std::string, size_t, std::vector<uint8_t>>>* SinObjectFile::get_data_table()
{
	return &this->data_table;
}

std::list<RelocationSymbol>* SinObjectFile::get_relocation_table()
{
	return &this->relocation_table;
}


// Class constructor and destructor

SinObjectFile::SinObjectFile() {
	// initialize all of these to 0 so they are never left uninitialized
	this->_wordsize = 0;
	this->_sinvm_version = 0;
	this->_text_start = 0;
}

SinObjectFile::SinObjectFile(std::istream& file) {
	// initialize these variables before calling "load" so they are never left uninitialized
	this->_wordsize = 0;
	this->_sinvm_version = 0;
	this->_text_start = 0;

	// load the sinc file
	this->load_sinc_file(file);
}

SinObjectFile::~SinObjectFile() {

}
