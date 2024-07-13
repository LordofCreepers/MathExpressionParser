/*
MIT License

Copyright (c) 2024 LordofCreepers

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "Parser/Exceptions.hpp"
#include "Parser/Parser.hpp"

// Thrown when number was formed incorrectly
class IncorrectlyFormedNumber : public SyntaxError
{
public:
	IncorrectlyFormedNumber(size_t character) : SyntaxError(character) {};

	virtual const char* what() const noexcept override
	{
		return "Incorrectly formed number";
	}
};

// Base for errors thrown during parsing stage
class ParsingError : public ExpressionError
{
	// Erroneous token
	const Parser::IToken* Token;
public:
	ParsingError(const Parser::IToken* token) : Token(token) {};

	virtual const Parser::IToken* GetToken() const
	{
		return Token;
	}
};

// Thrown when Pair token cannot find a suitable pair
class NoMatchingToken : public ParsingError
{
public:
	NoMatchingToken(const Parser::IToken* token) : ParsingError(token) {};

	virtual const char* what() const noexcept override
	{
		return "No matching token found";
	}
};

// OBSOLETE: Used to be thrown in 'Tokenize' when expression is empty
/* class EmptyExpression : public ExpressionError
{
public:
	virtual const char* what() const noexcept override
	{
		return "Empty expression";
	}
}; */

// OBSOLETE: I don't even remember where it is supposed to be thrown
/* class InvalidFactory : public ExpressionError
{
public:
	virtual const char* what() const noexcept override
	{
		return "Invalid factory";
	}
}; */

// Count of child nodes of an AST node mismatches from what it was expected to be
class UnexpectedSubexpressionCount : public ParsingError
{
protected:
	size_t CurCount, ExpCount;
public:
	UnexpectedSubexpressionCount(const Parser::IToken* token, size_t cur_count, size_t exp_count)
		: CurCount(cur_count), ExpCount(exp_count), ParsingError(token)
	{};

	virtual const char* what() const noexcept override
	{
		return "Mismatch between expected and provided amount of arguments";
	}

	virtual size_t CurrentCount() const
	{
		return CurCount;
	}

	virtual size_t ExpectedCount() const
	{
		return ExpCount;
	}
};

// Thrown when encountered a token that is not of expected type
class WrongTokenType : public ParsingError
{
public:
	WrongTokenType(const Parser::IToken* token) : ParsingError(token) {};

	virtual const char* what() const noexcept override
	{
		return "Wrong token type";
	}
};

// Thrown when any step of math expression evaluation results in division by zero
class DivisionByZero : public ParsingError
{
public:
	DivisionByZero(const Parser::IToken* token) : ParsingError(token) {};

	virtual const char* what() const noexcept override
	{
		return "Division by 0";
	}
};

/* Thrown when any step of math expression evaluation results in an attempt to take
the square root of a negative number
*/
class NegativeNumberRoot : public ParsingError
{
public:
	NegativeNumberRoot(const Parser::IToken* token) : ParsingError(token) {};

	virtual const char* what() const noexcept override
	{
		return "Extracting a root from a negative number";
	}
};

// Thrown when a value of a variable was not found in the environment table
class UnresolvedSymbol : public ParsingError
{
protected:
	const std::string& Symbol;
public:
	UnresolvedSymbol(const Parser::IToken* token, const std::string& symbol) : ParsingError(token), Symbol(symbol) {};

	virtual const char* what() const noexcept override
	{
		return "Unresolved symbol";
	}

	const std::string& GetSymbolName() const
	{
		return Symbol;
	}
};

class UnexpectedSeparator : public ParsingError
{
public:
	UnexpectedSeparator(const Parser::IToken* token) : ParsingError(token) {};

	virtual const char* what() const noexcept override
	{
		return "Param delimiter outside of any function";
	}
};