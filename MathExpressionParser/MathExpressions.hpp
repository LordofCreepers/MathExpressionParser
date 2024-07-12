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

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include "Parser/Parser.hpp"
#include "Parser/Tree.hpp"

// Boilerplate for constructor definition of tokens which constructors take in a range of source expression as a parameter
#define TOKEN_CONSTR_DEF(ClassName, ...) ClassName##(View<std::string>, ##__VA_ARGS__##)

namespace MathExpressions
{
	// Type that defines how variables are stored
	using Environment = std::unordered_map<std::string, long double>;

	// Token implementation that tracks where it has been sourced from
	struct SourcedToken : public Parser::IToken
	{
		View<std::string> Source;

		TOKEN_CONSTR_DEF(SourcedToken);
	};

	// Separator of function call parameters
	class ParamSeparator : public SourcedToken
	{
	public:
		TOKEN_CONSTR_DEF(ParamSeparator);

		virtual bool IsPrecedent(const Parser::IToken*) const override;

		virtual void FindNextToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Basic class for all math operations, constants and functions
	class Token : public SourcedToken
	{
	protected:
		// Extracts values of this token's child tokens down the ast
		void EvaluateChildren(
			const Tree<Parser::TokenPtr>::NodePtr&,
			std::vector<long double>&,  
			const Environment&,
			size_t
		) const;
	public:
		TOKEN_CONSTR_DEF(Token);

		/* Returns priority of this operation or function
		The lower the priority, the higher up the tree token is
		*/
		virtual size_t GetPriority() const = 0;

		/* Mathematically evaluates this token. This can be simply retrieving value of a number or applying
		operation to it's children
		*/
		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&, 
			const Environment&
		) const = 0;

		virtual bool IsPrecedent(const Parser::IToken*) const override;

		virtual void FindNextToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Base for numeric tokens (e.g. numbers and constants)
	class Numeric : public Token
	{
	public:
		TOKEN_CONSTR_DEF(Numeric);

		virtual size_t GetPriority() const override;

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Number ("123.45" and the like)
	class Number : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Number);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Pi
	class Pythagorean : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Pythagorean);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Euler's number
	class ExponentConst : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(ExponentConst);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Variable
	class Variable : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Variable);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Basic class for any binary operation - an operation that takes two children and combines their values in a specific way
	class BinaryOp : public Token
	{
	public:
		TOKEN_CONSTR_DEF(BinaryOp);

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Addition
	class Add : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Add);

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Subtraction
	class Sub : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Sub);

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Multiplication
	class Mul : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Mul);

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Division
	class Div : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Div);

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Power
	class Pow : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Pow);

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Basic class for a token that needs to have some sort of pair in expression it's in
	class Pair : public Token
	{
	public:
		TOKEN_CONSTR_DEF(Pair);

		virtual void FindNextToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;

		virtual void FindMatchingToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const = 0;

		// Checks if token passed as parameter is matching to current token
		virtual bool IsMatchingToken(const Pair*) const = 0;
	};

	// Basic class for pairs that has their first token distinct from the second
	class DistinctPair : public Pair
	{
	public:
		/* The "variant" of this token
		Currently build framework implies each DistinctPair can only have
		one variant of it's matching token
		As of this, 'false' represents the first token, and 'true' represents the second
		*/
		bool Variant;

		TOKEN_CONSTR_DEF(DistinctPair, bool);

		virtual void FindMatchingToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Opening and closing brackets
	class Bracket : public DistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(Bracket, bool);

		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Base class for pairs of identical tokens
	class IndistinctPair : public Pair
	{
	public:
		TOKEN_CONSTR_DEF(IndistinctPair);

		virtual void FindMatchingToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Modulo brackets ('|')
	class ModBracket : public IndistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(ModBracket);

		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Function : public DistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(Function);
			
		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;
	};

	// Logarithm with the base of E
	class LogarithmE : public Function
	{
	public:
		TOKEN_CONSTR_DEF(LogarithmE);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Logarithm with the base of 2
	class Logarithm2 : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm2);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Logarithm with the base of 10
	class Logarithm10 : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm10);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Function with multiple arguments
	class ArgumentedFunction : public Function
	{
	public:
		TOKEN_CONSTR_DEF(ArgumentedFunction);

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Logarithm with dynamic base
	class Logarithm : public ArgumentedFunction
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Euler's number raised to a power
	class ExponentFunc : public Function
	{
	public:
		TOKEN_CONSTR_DEF(ExponentFunc);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class SquareRoot : public Function
	{
	public:
		TOKEN_CONSTR_DEF(SquareRoot);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Sign (-1 for negative values, 1 for positive values, 0 for 0)
	class Sign : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Sign);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Sine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Sine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Cosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Cosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Tangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Tangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Cotangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Cotangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arcsine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arcsine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arccosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arccosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arctangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arctangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicSine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicSine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicCosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicCosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicTangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicTangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArcsine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArcsine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArccosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArccosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArctangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArctangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	const std::vector<Parser::TokenFactory>& GetTokenFactories();
}