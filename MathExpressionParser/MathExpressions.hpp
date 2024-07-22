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

		virtual void Stringify(
			View<std::vector<Parser::TokenPtr>> tokens,
			std::vector<Parser::TokenPtr>::const_iterator cur_token,
			std::string& out_expression
		) const override;
		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;

		virtual void Backpatch(
			std::vector<Parser::TokenPtr>& tokens,
			std::vector<Parser::TokenPtr>::iterator cur_token
		) override;
		virtual void Backpatch(
			Tree<Parser::TokenPtr>& tree,
			Tree<Parser::TokenPtr>::Node& cur_node
		) override;
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
		/// <summary>
		/// Extracts values of this token's child tokens down the ast
		/// </summary>
		/// <param name="cur_node">- node in the tree being evaluated with it's associated token
		/// being this one</param>
		/// <param name="out_children">- calculated values for this token's children</param>
		/// <param name="env">- registry of variable values</param>
		/// <param name="expected_param_count">- a number of expected children.
		/// If non-zero, automatically throws UnexpectedSubexpressionCount 
		/// if number of children doesn't match this</param>
		void EvaluateChildren(
			const Tree<Parser::TokenPtr>::NodePtr& cur_node,
			std::vector<long double>& out_children,  
			const Environment& env,
			size_t expected_param_count
		) const;
	public:
		TOKEN_CONSTR_DEF(Token);

		/// <summary>
		/// Returns priority of this operation or function
		/// The lower the priority, the higher up the tree token is
		/// </summary>
		/// <returns>Numeric representation of this token's priority</returns>
		virtual size_t GetPriority() const = 0;

		/// <summary>
		/// Mathematically evaluates this token. This can be simply retrieving value of a number or applying
		/// operation to it's children
		/// </summary>
		/// <param name="cur_node">- node in the tree being evaluated with it's associated token
		/// being this one</param>
		/// <param name="env">- registry of variable values</param>
		/// <returns>Result of calculation</returns>
		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr& cur_node, 
			const Environment& env
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

	/* Number 
	Tries to evaluate 'A' to it's numering value,
	where 'A' - a string composed of only digits and (optionally) a dot
	*/
	class Number : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Number);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	/* Pi
	Evaluates itself to number 'Pi'
	*/
	class Pythagorean : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Pythagorean);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	/* Euler's number
	Evaluates itself to Euler's number
	*/ 
	class ExponentConst : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(ExponentConst);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	/* Variable
	Tries to evaluate 'A' to it's assigned numeric value,
	where 'A' - string composed of latin alphabetic characters
	Looks up it's assigned value in the environment. If it doesn't find
	itself, throws UnresolvedSymbol
	*/ 
	class Variable : public Numeric
	{
	public:
		TOKEN_CONSTR_DEF(Variable);

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	/* Basic class for any binary operation - an operation that takes two children and combines their values in a specific way
	Tries to evaluate 'A op B',
	where 'A' and 'B' - any type of token,
	'op' - operation that processes these tokens.
	If either 'A' or 'B' is not present, throws UnexpectedSubexpressionCount
	*/
	class BinaryOp : public Token
	{
	public:
		TOKEN_CONSTR_DEF(BinaryOp);

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;
	};

	/* Addition
	Evaluates 'A + B' by adding result of 'A' to the result of 'B'
	*/ 
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

	/* Subtraction
	Evaluates 'A - B' by subtracting result of 'B' from the result of 'A'
	Is an exception to BinaryOp operating rules, as in
	'- A' is a valid expression and evaluates to negation of 'A' (inverts sign)
	Does not throw UnexpectedSubexpressionCount if subexpression count is 1
	*/
	class Sub : public BinaryOp
	{
	public:
		TOKEN_CONSTR_DEF(Sub);

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;

		virtual size_t GetPriority() const override;

		virtual long double Evaluate(
			const Tree<Parser::TokenPtr>::NodePtr&,
			const Environment&
		) const override;
	};

	/* Multiplication
	Evaluates 'A * B' by multiplying result of 'A' by the result of 'B'
	*/
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

	/* Division
	Evaluates 'A / B' by adding result of 'A' to the result of 'B'
	*/
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

	/* Power
	Evaluates 'A ^ B' by raising result of 'A' to the result of 'B'
	*/
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

	/* Basic class for a token that needs to have some sort of pair in expression it's in
	Tries to evaluate 'pair...pair',
	where 'pair' is this token type,
	'...' - any combination of tokens inbetween them.
	If there are no subexpressions inbetween the pair, throws UnexpectedSubexpressionCount
	*/ 
	class Pair : public Token
	{
	public:
		/* Data structure to keep found pair's positions
		Key is a pointer to the vector where the pair was found
		Value is an iterator set to found pair
		*/
		using PairCacheMap = std::unordered_map<
			const std::vector<Parser::TokenPtr>*,
			std::vector<Parser::TokenPtr>::const_iterator
		>;
	protected:
		PairCacheMap PairCache;

		/// <summary>
		/// Tries to lookup the matching token in the view
		/// </summary>
		/// <param name="tokens">- view of an array of tokens where matching token should be located</param>
		/// <param name="cur_token">- location of current token in that vector</param>
		/// <param name="safe">- if true, does not throw NoMatchingToken if a match isn't found</param>
		virtual void LookupMatchingToken(
			View<std::vector<Parser::TokenPtr>> tokens,
			std::vector<Parser::TokenPtr>::const_iterator& cur_token,
			bool safe
		) const = 0;
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

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;

		virtual void Backpatch(
			std::vector<Parser::TokenPtr>& tokens,
			std::vector<Parser::TokenPtr>::iterator cur_token
		) override;

		/// <summary>
		/// Tries to find the matching token in a view
		/// Simply delegates to 'LookupMatchingToken' with 'safe' set to false
		/// </summary>
		/// <param name="tokens">- view of an array of tokens where matching token should be located</param>
		/// <param name="cur_token">- location of current token in that vector</param>
		void FindMatchingToken(
			View<std::vector<Parser::TokenPtr>> tokens,
			std::vector<Parser::TokenPtr>::const_iterator& cur_token
		) const;

		/// <summary>
		/// Checks if token passed as parameter is matching to current token
		/// </summary>
		/// <param name="other">- other token</param>
		/// <returns>Whether 'other' can be considered a match</returns>
		virtual bool IsMatchingToken(const Pair* other) const = 0;
	};

	// Basic class for pairs that has their first token distinct from the second
	class DistinctPair : public Pair
	{
	protected:
		virtual void LookupMatchingToken(
			View<std::vector<Parser::TokenPtr>> tokens,
			std::vector<Parser::TokenPtr>::const_iterator& cur_token,
			bool safe
		) const override;
	public:
		/* The "variant" of this token
		Currently build framework implies each DistinctPair can only have
		one variant of it's matching token
		As of this, 'false' represents the first token, and 'true' represents the second
		*/
		bool Variant;

		TOKEN_CONSTR_DEF(DistinctPair, bool);
	};

	/* Opening and closing brackets
	Evaluates expression '(A)' to... just 'A'. That's it
	These, however, have high priority, therefore they implicitly
	shift the priority of their subexpressions
	*/
	class Bracket : public DistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(Bracket, bool);

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;

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
	protected:
		virtual void LookupMatchingToken(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator&,
			bool
		) const override;
	public:
		TOKEN_CONSTR_DEF(IndistinctPair);
	};

	// Modulo brackets ('|'). Work much like normal brackets, but evaluate the absolute value of the subexpressions
	class ModBracket : public IndistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(ModBracket);

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;

		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Base class for functions
	Evaluates 'A(...)',
	where 'A' - function identifier,
	'...' - function parameters (depend on the function itself)
	If parameter count do not match the expected amount, throws UnexpectedSubexpressionCount
	*/
	class Function : public DistinctPair
	{
	public:
		TOKEN_CONSTR_DEF(Function);

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;
			
		virtual size_t GetPriority() const override;

		virtual bool IsMatchingToken(const Pair*) const override;
	};

	/* Logarithm with the base of E
	Evaluates 'ln(A)' to natural logarithm of 'A'
	*/
	class LogarithmE : public Function
	{
	public:
		TOKEN_CONSTR_DEF(LogarithmE);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Logarithm with the base of 2
	Evaluates 'log2(A)' to logarithm of 'A' with the base of 2
	*/
	class Logarithm2 : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm2);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Logarithm with the base of 10
	Evaluates 'log10(A)' to logarithm of 'A' with the base of 10
	*/
	class Logarithm10 : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm10);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	// Base for function with multiple arguments
	class ArgumentedFunction : public Function
	{
	public:
		TOKEN_CONSTR_DEF(ArgumentedFunction);

		virtual void SplitPoints(
			View<std::vector<Parser::TokenPtr>>,
			std::vector<Parser::TokenPtr>::const_iterator,
			std::vector<View<std::vector<Parser::TokenPtr>>>&
		) const override;

		virtual void Stringify(
			const Tree<Parser::TokenPtr>& tree,
			const Tree<Parser::TokenPtr>::Node& cur_node,
			std::string& out_expression
		) const override;
	};

	/* Logarithm with dynamic base
	Evaluates 'log(A, B)' to logarithm of 'A' with the base of 'B',
	where 'A' and 'B' - any tokens
	*/
	class Logarithm : public ArgumentedFunction
	{
	public:
		TOKEN_CONSTR_DEF(Logarithm);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Euler's number raised to a power
	Evaluates 'exp(A)' by raising 'e' to the power of 'A'
	*/
	class ExponentFunc : public Function
	{
	public:
		TOKEN_CONSTR_DEF(ExponentFunc);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Square root
	Evaluates 'sqrt(A)' by taking the square root of 'A'
	If 'A' evaluates to a negative number, throws NegativeNumberRoot
	*/
	class SquareRoot : public Function
	{
	public:
		TOKEN_CONSTR_DEF(SquareRoot);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Sign
	Evaluates 'sign(A)' to either 
	* 1 if 'A' evaluates to positive number, 
	* 0 if 'A' evaluates to 0 or 
	* -1 if 'A' evaluates to negative number
	*/
	class Sign : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Sign);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Sine
	Evaluates 'sin(A)' to the sine of 'A'
	*/
	class Sine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Sine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Cosine
	Evaluates 'cos(A)' to the cosine of 'A'
	*/
	class Cosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Cosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Tangent
	Evaluates 'tan(A)' or 'tg(A)' to the tangent of 'A'
	*/
	class Tangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Tangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Cotangent
	Evaluates 'ctan(A)' or 'ctg(A)' to the cotangent of 'A'
	*/
	class Cotangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Cotangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Arcsine
	Evaluates 'asin(A)' or 'arcsin(A)' to the arcsine of 'A'
	*/
	class Arcsine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arcsine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Arccosine
	Evaluates 'acos(A)' or 'arccos(A)' to the arccosine of 'A'
	*/
	class Arccosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arccosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Arctangent
	Evaluates 'atg(A)', 'atan(A)', 'arctg(A)' or 'arctan(A)' to arctangent of 'A'
	*/
	class Arctangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(Arctangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic sine
	Evaluates 'sinh(A)' to hyperbolic sine of 'A'
	*/
	class HyperbolicSine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicSine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic cosine
	Evaluates 'cosh(A)' to hyperbolic cosine of 'A'
	*/
	class HyperbolicCosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicCosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic tangent
	Evaluates 'tgh(A)' or 'tanh(A)' to hyperbolic tangent of 'A'
	*/
	class HyperbolicTangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicTangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic arcsine
	Evaluates 'asinh(A)' or 'arcsinh(A)' to hyperbolic arcsine of 'A'
	*/
	class HyperbolicArcsine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArcsine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic arccosine
	Evaluates 'acosh(A)' or 'arccosh(A)' to hyperbolic arccosine of 'A'
	*/
	class HyperbolicArccosine : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArccosine);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/* Hyperbolic arctangent
	Evaluates 'atgh(A)', 'atanh(A)', 'arctgh(A)' or 'arctanh(A)' to hyperbolic arctangent of 'A'
	*/
	class HyperbolicArctangent : public Function
	{
	public:
		TOKEN_CONSTR_DEF(HyperbolicArctangent);

		virtual long double Evaluate(const Tree<Parser::TokenPtr>::NodePtr&, const Environment&) const override;
	};

	/// <summary>
	/// Shorthand that returns all factories needed for parser to parse mathematical expressions
	/// </summary>
	const std::vector<Parser::TokenFactory>& GetTokenFactories();

	/// <summary>
	/// Shorthand that tokenizes, parses and evaluates expression in provided string and environment
	/// </summary>
	long double Evaluate(const std::string&, const Environment&, std::vector<Parser::TokenPtr>&, Tree<Parser::TokenPtr>&);
}