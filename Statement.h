#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>

#include "Expression.h"


// use an enum to track statement types instead of strings
enum stmt_type {
	STATEMENT_GENERAL,
	INCLUDE,
	ALLOCATION,
	ASSIGNMENT,
	RETURN_STATEMENT,
	IF_THEN_ELSE,
	WHILE_LOOP,
	DEFINITION,
	CALL,
	INLINE_ASM
};


// Statement is the base class for all statements

class Statement
{
protected:
	stmt_type statement_type;	// tells us whether its an Allocation, and Assignment, an ITE...
	std::string scope_name;	// to track the scope name under which the statement is being executed
	int scope_level;	// to track the scope level
	int line_number;	// the line number on which the first token of the statement can be found in the file

	// TODO: add scope information to statements in Parser

public:
	stmt_type get_statement_type();
	std::string get_scope_name();
	int get_scope_level();
	int get_line_number();
	void set_line_number(int line_number);

	Statement();
	virtual ~Statement();
};

class StatementBlock
{
public:
	std::vector<std::shared_ptr<Statement>> statements_list;

	StatementBlock();
	~StatementBlock();
};

class Include : public Statement
{
	std::string filename;
public:
	std::string get_filename();
	Include(std::string filename);
	Include();
};

class Allocation : public Statement
{
	/*
	
	For a statement like:
		alloc int myInt;
	we create an allocation statement like so:
		type			:	INT
		value			:	myInt
		initialized		:	false
		initial_value	:	(none) 

	We can also use what is called "alloc-define syntax" in SIN:
		alloc int myInt: 5;
	which will allocate the variable and make an initial assignment. In this case, the allocation looks like:
		type			:	INT
		value			:	myInt
		initialized		:	true
		initial_value	:	5

	This "alloc-define" syntax is required for all const-qualified data types

	*/
	
	Type type;	// the variable's type
	Type subtype;	// the subtype
	std::string value;

	std::string quality;	// the "quality" of the variable (defaults to "none", but can be const, etc)

	// If we have an alloc-define statement, we will need:
	bool initialized;	// whether the variable was defined upon allocation

	std::shared_ptr<Expression> initial_value;
public:
	Type get_var_type();
	Type get_var_subtype();
	static std::string get_var_type_as_string(Type to_convert);
	std::string get_var_name();

	std::string get_quality();

	bool was_initialized();
	std::shared_ptr<Expression> get_initial_value();

	Allocation(Type type, std::string value, Type subtype = NONE, bool was_initialized = false, std::shared_ptr<Expression> initial_value = std::make_shared<Expression>(), std::string quality = "none");	// use default parameters to allow us to use alloc-define syntax, but we don't have to
	Allocation();
};

class Assignment : public Statement
{
	LValue lvalue;
	std::shared_ptr<Expression> rvalue_ptr;
public:
	// get the variable names / expression types
	std::string get_lvalue_name();
	exp_type get_rvalue_expression_type();

	// get the variables / expressions themselves
	LValue get_lvalue();
	std::shared_ptr<Expression> get_rvalue();

	Assignment(LValue lvalue, std::shared_ptr<Expression> rvalue);
	Assignment();
};

class ReturnStatement : public Statement
{
	std::shared_ptr<Expression> return_exp;
public:
	std::shared_ptr<Expression> get_return_exp();

	ReturnStatement(std::shared_ptr<Expression> exp_ptr);
	ReturnStatement();
};

class IfThenElse : public Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<StatementBlock> if_branch;
	std::shared_ptr<StatementBlock> else_branch;
public:
	std::shared_ptr<Expression> get_condition();
	std::shared_ptr<StatementBlock> get_if_branch();
	std::shared_ptr<StatementBlock> get_else_branch();

	IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr, std::shared_ptr<StatementBlock> else_branch_ptr);
	IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<StatementBlock> if_branch_ptr);
	IfThenElse();
};

class WhileLoop : public Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<StatementBlock> branch;
public:
	std::shared_ptr<Expression> get_condition();
	std::shared_ptr<StatementBlock> get_branch();

	WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> branch);
	WhileLoop();
};

class Definition : public Statement
{
	std::shared_ptr<Expression> name;
	Type return_type;
	std::vector<std::shared_ptr<Statement>> args;
	std::shared_ptr<StatementBlock> procedure;

	// TODO: add function qualities? currently, definitions just put "none" for the symbol's quality
public:
	std::shared_ptr<Expression> get_name();
	Type get_return_type();
	std::shared_ptr<StatementBlock> get_procedure();
	std::vector<std::shared_ptr<Statement>> get_args();

	Definition(std::shared_ptr<Expression> name_ptr, Type return_type_ptr, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr);
	Definition();
};

class Call : public Statement
{
	std::shared_ptr<LValue> func;	// the function name
	std::vector<std::shared_ptr<Expression>> args;	// arguments to the function
public:
	std::string get_func_name();
	int get_args_size();
	std::shared_ptr<Expression> get_arg(int num);

	Call(std::shared_ptr<LValue> func, std::vector<std::shared_ptr<Expression>> args);
	Call();
};

class InlineAssembly : public Statement
{
	std::string asm_type;
public:
	std::string get_asm_type();

	std::string asm_code;

	InlineAssembly(std::string asm_type, std::string asm_code);
	InlineAssembly();
};
