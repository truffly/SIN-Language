/*

SIN Toolchain
SymbolTable.cpp
Copyright 2019 Riley Lannon

The implementation of the SymbolTable class.

*/


#include "SymbolTable.h"


// Our SymbolTable object

void SymbolTable::insert(std::string name, Type type, std::string scope_name, size_t scope_level, Type sub_type, std::vector<SymbolQuality> qualities, bool initialized, std::vector<std::shared_ptr<Statement>> formal_parameters, unsigned int line_number)
{	
	if (this->exists_in_scope(name, scope_name, scope_level)) {
		throw SymbolTableException("'" + name + "'already in symbol table.", line_number);
	}
	else {
		this->symbols.push_back(Symbol(name, type, scope_name, scope_level, sub_type, qualities, initialized, formal_parameters));	// an allocation is NOT a definition
	}
}

void SymbolTable::insert(Symbol to_add, unsigned int line_number) {
	if (this->exists_in_scope(to_add.name, to_add.scope_name, to_add.scope_level)) {
		throw SymbolTableException("'" + to_add.name + "'already in symbol table.", line_number);
	}
	else {
		this->symbols.push_back(to_add);	// an allocation is NOT a definition
	}
}

void SymbolTable::define(std::string symbol_name, std::string scope_name)
{
	if (is_in_symbol_table(symbol_name, scope_name)) {

	} else {
		throw SymbolTableException("Cannot find allocation for " + symbol_name);
	}
}



void SymbolTable::remove(std::string symbol_name, std::string scope_name, size_t scope_level) {
	/*
	
	Intended for use in local scopes, specifically ITE and While loops to remove any symbols that were declared within. This way, they cannot be accessed in scopes of the same level (or higher) that are not within that block.

	Iterates through the symbol table, checking for a variable matching the symbol name within the scope and level specified, and removes it if it finds one.

	*/

	std::vector<Symbol>::iterator symbol_iter = this->symbols.begin();
	
	while (symbol_iter != this->symbols.end()) {
		if ((symbol_iter->name == symbol_name) && (symbol_iter->scope_name == scope_name) && (symbol_iter->scope_level == scope_level)) {
			// remove the symbol, but do not increment the symbol_iter; it now will point to the element after the one we just erased
			this->symbols.erase(symbol_iter);
		}
		else {
			symbol_iter++;
		}
	}
}



Symbol* SymbolTable::lookup(std::string symbol_name, std::string scope_name, size_t scope_level)
{
	/*
	
	Returns a pointer to a symbol in the table with the specified name, scope name, and scope level
	The function will try to find the most recently declared variable, but will use variables in wider scopes if it must.
	
	*/
	
	// iterate through our vector
	std::vector<Symbol>::iterator symbols_iter = this->symbols.begin();
	Symbol* to_return = nullptr;
	bool found = false;

	while (symbols_iter != this->symbols.end()) {

		// if our name is in the symbol table
		if (symbol_name == symbols_iter->name) {
			// the first time we find a symbol, let to_return point to that element
			if (!found) {
				found = true;
				to_return = &*symbols_iter;
			}
			else {
				// only add matches where the scope name is the scope name supplied, or it is in the lowest global scope if we aren't looking in global
				if ((symbols_iter->scope_name == scope_name) || (symbols_iter->scope_name == "global" && symbols_iter->scope_level == 0)) {
					// now, check to see if the current symbol is in a higher scope than to_return; we want the variable declared most recently
					if (symbols_iter->scope_level > to_return->scope_level) {
						to_return = &*symbols_iter;	// to_return should not point to that symbol
					}
				}
			}
		}

		// advance the iterator
		symbols_iter++;
	}

	if (found) {
		return to_return;
	}
	else {
		throw SymbolTableException("Cannot find '" + symbol_name + "' in symbol table!");
	}
}

bool SymbolTable::is_in_symbol_table(std::string symbol_name, std::string scope_name)
{
	/*
	
	Checks to see if a symbol with the name 'symbol_name' is in scope 'scope_name'. Will return true if this is the case. The function will also return true if it finds a symbol with the specified name in the global scope; this function is simply meant to allow a user to check and see if there is a variable with some name in the Compiler's symbol table.
	
	*/
	
	// iterate through our vector
	bool found = false;
	std::vector<Symbol>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// if we have an entry in the same scope of the same name, or if it is a static global symbol
		if ((symbol_name == iter->name) && ((scope_name == iter->scope_name) || (iter->scope_name == "global" && iter->scope_level == 0))) {
			found = true;	// set our 'found' flag to true
		} else {
			iter++;	// increment the iterator
		}
	}

	// return our 'found' flag
	return found;
}

bool SymbolTable::exists_in_scope(std::string symbol_name, std::string scope_name, size_t scope_level)
{
	/*
	
	Checks to see whether a symbol of a given name already exists in the exact scope specified; this is used by the SymbolTable::insert(...) function to ensure the symbol we want to add doesn't already exist in a scope at a specific scope level

	*/
	
	bool found = false;
	std::vector<Symbol>::iterator iter = this->symbols.begin();

	while ((iter != this->symbols.end()) && !found) {
		// check to see if the symbol names, scopes, and levels match
		if (iter->name == symbol_name && iter->scope_name == scope_name && iter->scope_level == scope_level) {
			found = true;
		}
		else {
			iter++;	// advance the iterator
		}
	}

	return found;
}

SymbolTable::SymbolTable()
{
}


SymbolTable::~SymbolTable()
{
}