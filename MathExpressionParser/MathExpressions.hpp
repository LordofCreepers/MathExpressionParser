#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include "Parser/Parser.hpp"
#include "Parser/Tree.hpp"

namespace MathExpressions
{
	// Type that defines how variables are stored
	using Environment = std::unordered_map<std::string, long double>;

	// Separator of function call parameters
	class ParamSeparator : public Parser::IToken
	{
		virtual bool IsPrecedent(const Parser::IToken*) const override;

		virtual void FindNextToken(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;

		virtual void SplitPoints(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<Range<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Basic class for all math operations, constants and functions
	class Token : public Parser::IToken
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
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Base for numeric tokens (e.g. numbers and constants)
	class Numeric : public Token
	{
	public:
		virtual size_t GetPriority() const override;

		virtual void SplitPoints(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<Range<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Number ("123.45" and the like)
	class Number : public Numeric
	{
	protected:
		// Number represented by string
		std::string Num;
	public:
		Number(const std::string&);
		Number(std::string&&);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Pi
	class Pythagorean : public Numeric
	{
	public:
		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Euler's number
	class ExponentConst : public Numeric
	{
	public:
		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Variable (in expression '2x' 'x' is the variable)
	class Variable : public Numeric
	{
	protected:
		// Variable name
		std::string Name;
	public:
		Variable(char);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	// Basic class for any binary operation - an operation that takes two children and combines their values in a specific way
	class BinaryOp : public Token
	{
	public:
		virtual void SplitPoints(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<Range<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Addition
	class Add : public BinaryOp
	{
	public:
		Add() = default;

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
		Sub() = default;

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
		Mul() = default;

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
		Div() = default;

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
		Pow() = default;

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
		Pair() = default;

		virtual void FindNextToken(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;

		virtual void SplitPoints(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<Range<std::vector<Parser::TokenPtr>>>&
		) const override;

		virtual void FindMatchingToken(
			Range<std::vector<Parser::TokenPtr>>,
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

		DistinctPair(bool);

		virtual void FindMatchingToken(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Opening and closing brackets
	class Bracket : public DistinctPair
	{
	public:
		Bracket(bool);

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
		virtual void FindMatchingToken(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&
		) const override;
	};

	// Modulo brackets ('|')
	class ModBracket : public IndistinctPair
	{
	public:
		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Function : public DistinctPair
	{
	public:
		Function();

		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;
	};

	// Logarithm with the base of E
	class LogarithmE : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Logarithm with the base of 2
	class Logarithm2 : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Logarithm with the base of 10
	class Logarithm10 : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Function with multiple arguments
	class ArgumentedFunction : public Function
	{
	public:
		virtual void SplitPoints(
			Range<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<Range<std::vector<Parser::TokenPtr>>>&
		) const override;
	};

	// Logarithm with dynamic base
	class Logarithm : public ArgumentedFunction
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Euler's number raised to a power
	class ExponentFunc : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class SquareRoot : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Sign (-1 for negative values, 1 for positive values, 0 for 0)
	class Sign : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Sine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Cosine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Tangent : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Cotangent : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arcsine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arccosine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class Arctangent : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicSine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicCosine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicTangent : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArcsine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArccosine : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	class HyperbolicArctangent : public Function
	{
	public:
		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	const std::vector<Parser::TokenFactory>& GetTokenFactories();
}