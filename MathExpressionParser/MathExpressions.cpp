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

#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <unordered_set>
#include "Exceptions.hpp"
#include "MathExpressions.hpp"

// Boilerplate for constructor implementation of tokens that take substring of expression as their constructor's first parameter
#define TOKEN_CONSTR_IMPL(ClassName, BaseClass) MathExpressions::##ClassName##::##ClassName##(View<std::string> source_range \
) : ##BaseClass##(source_range) {}

MathExpressions::SourcedToken::SourcedToken(
    View<std::string> source_range
) : Source(source_range) {};

void MathExpressions::SourcedToken::Stringify(
    View<std::vector<Parser::TokenPtr>> tokens,
    std::vector<Parser::TokenPtr>::const_iterator cur_token,
    std::string& out_expression
) const {
    out_expression += std::string(Source.Start, Source.End);
}

void MathExpressions::SourcedToken::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    out_expression += std::string(Source.Start, Source.End);
}

void MathExpressions::SourcedToken::Backpatch(
    std::vector<Parser::TokenPtr>& tokens,
    std::vector<Parser::TokenPtr>::iterator cur_token
) {};

void MathExpressions::SourcedToken::Backpatch(
    Tree<Parser::TokenPtr>& tree,
    Tree<Parser::TokenPtr>::Node& cur_node
) {};

TOKEN_CONSTR_IMPL(Token, SourcedToken);

void MathExpressions::Token::EvaluateChildren(
    const Tree<Parser::TokenPtr>::NodePtr& ast_node,
    std::vector<long double>& out_params,
    const MathExpressions::Environment& env,
    size_t expected_param_count = 0
) const {
    // Throws if expected parameter count does not match what node currently has
    if (expected_param_count && ast_node->Children.size() != expected_param_count)
        throw UnexpectedSubexpressionCount(this, expected_param_count, ast_node->Children.size());

    // Takes children of provided node and evaluate them one by one, then puts values in an array
    for (const Tree<Parser::TokenPtr>::NodePtr& node : ast_node->Children)
    {
        // Can't evaluate if several token do not belong to category of math expression tokens
        auto token = std::dynamic_pointer_cast<const MathExpressions::Token>(node->Value);
        if (!token) throw WrongTokenType(node->Value.get());

        out_params.push_back(token->Evaluate(node, env));
    }
}

bool MathExpressions::Token::IsPrecedent(const Parser::IToken* other) const
{
    /* Can't definitively tell the precedence between two tokens if 
    they don't belong to category of math expression tokens
    */
    auto other_met = dynamic_cast<const MathExpressions::Token*>(other);
    if (!other_met) throw WrongTokenType(other);

    // Whichever priority is higher is precedent
    return GetPriority() > other_met->GetPriority();
}

void MathExpressions::Token::FindNextToken(
    View<std::vector<Parser::TokenPtr>>, 
    std::vector<Parser::TokenPtr>::const_iterator& out_ptr
) const {
    // By default, returns the following token in the array
    out_ptr++;
}

TOKEN_CONSTR_IMPL(Numeric, Token);

void MathExpressions::Numeric::SplitPoints(
    View<std::vector<Parser::TokenPtr>> expression_range,
    std::vector<Parser::TokenPtr>::const_iterator current_token,
    std::vector<View<std::vector<Parser::TokenPtr>>>& out_partition
) const {
    // Numeric tokens do not depend on any children
    out_partition.clear();
}

size_t MathExpressions::Numeric::GetPriority() const
{
    // Numerics should be evaluated first, so they have the biggest priority
    return std::numeric_limits<size_t>::max();
}

TOKEN_CONSTR_IMPL(Number, Numeric);

long double MathExpressions::Number::Evaluate(
    const Tree<Parser::TokenPtr>::NodePtr& node, 
    const MathExpressions::Environment& env
) const {
    return std::stold(std::string(Source.Start, Source.End));
}

TOKEN_CONSTR_IMPL(Pythagorean, Numeric);

long double MathExpressions::Pythagorean::Evaluate(
    const Tree<Parser::TokenPtr>::NodePtr&, 
    const MathExpressions::Environment&
) const {
    return M_PI;
}

TOKEN_CONSTR_IMPL(ExponentConst, Numeric);

long double MathExpressions::ExponentConst::Evaluate(
    const Tree<Parser::TokenPtr>::NodePtr&, 
    const MathExpressions::Environment&
) const {
    return M_E;
}

TOKEN_CONSTR_IMPL(Variable, Numeric);

long double MathExpressions::Variable::Evaluate(
    const Tree<Parser::TokenPtr>::NodePtr&, 
    const MathExpressions::Environment& env
) const {
    std::string var_name(Source.Start, Source.End);
    // Finds the value of the variable in 'env'
    Environment::const_iterator var_it = env.find(var_name);
    if (var_it == env.cend()) throw UnresolvedSymbol(this, var_name);

    return var_it->second;
}

TOKEN_CONSTR_IMPL(BinaryOp, Token);

void MathExpressions::BinaryOp::SplitPoints(
    View<std::vector<Parser::TokenPtr>> expression_range,
    std::vector<Parser::TokenPtr>::const_iterator current_token,
    std::vector<View<std::vector<Parser::TokenPtr>>>& out_partition
) const
{
    out_partition.clear();

    if (expression_range.Start == expression_range.End) return;

    // Binary operations simply split expression into two parts - everything to the right of them
    // and everything to the left
    out_partition.push_back({ expression_range.Source, expression_range.Start, current_token });
    out_partition.push_back({ expression_range.Source, current_token + 1, expression_range.End });
}

void MathExpressions::BinaryOp::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    if (cur_node.Children.size() != 2) throw UnexpectedSubexpressionCount(this, cur_node.Children.size(), 2);

    Tree<Parser::TokenPtr>::Node& lh_child = *cur_node.Children[0];
    lh_child.Value->Stringify(tree, lh_child, out_expression);

    SourcedToken::Stringify(tree, cur_node, out_expression);

    Tree<Parser::TokenPtr>::Node& rh_child = *cur_node.Children[1];
    rh_child.Value->Stringify(tree, rh_child, out_expression);
}

TOKEN_CONSTR_IMPL(Add, BinaryOp);

size_t MathExpressions::Add::GetPriority() const
{
    return 1;
}

long double MathExpressions::Add::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    // Breaks when adding less than 2 operands
    if (node->Children.size() < 2) throw UnexpectedSubexpressionCount(this, node->Children.size(), 2);

    // Evaluates it's subnodes and adds-up the result
    // Supports more than two parameters, but it shouldn't
    // be possible unless you change SplitPoints to gather more ranges

    // Operation result
    long double res = 0;
    // Evaluated children values
    std::vector<long double> params;
    EvaluateChildren(node, params, env);
    
    for (long double param : params)
        res += param;

    return res;
}

TOKEN_CONSTR_IMPL(Sub, BinaryOp);

void MathExpressions::Sub::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    if (cur_node.Children.empty()) throw UnexpectedSubexpressionCount(this, 0, 1);
    if (cur_node.Children.size() > 2) throw UnexpectedSubexpressionCount(this, cur_node.Children.size(), 2);

    if (cur_node.Children.size() > 1)
    {
        Tree<Parser::TokenPtr>::Node& lh_child = *cur_node.Children[0];
        lh_child.Value->Stringify(tree, lh_child, out_expression);
    }

    SourcedToken::Stringify(tree, cur_node, out_expression);

    Tree<Parser::TokenPtr>::Node& rh_child = *cur_node.Children[cur_node.Children.size() > 1 ? 1 : 0];
    rh_child.Value->Stringify(tree, rh_child, out_expression);
}

size_t MathExpressions::Sub::GetPriority() const
{
    return 1;
}

long double MathExpressions::Sub::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    // Same as above
    if (node->Children.empty()) throw UnexpectedSubexpressionCount(this, node->Children.size(), 1);

    std::vector<long double> params;
    EvaluateChildren(node, params, env);

    // Special rule for when minus sign is placed in front of another token without any applicable
    // preciding one, equating to negating operation
    if (params.size() == 1) return -params[0];

    // This time, because subtraction is not symmetrical operation,
    // first parameter is the initial value and everything else is being subtracted
    // from it

    // Operation result. Initialized to the first parameter
    long double res = params[0];
    for (size_t i = 1; i < params.size(); i++)
        res -= params[i];

    return res;
}

TOKEN_CONSTR_IMPL(Mul, BinaryOp);

size_t MathExpressions::Mul::GetPriority() const
{
    return 2;
}

long double MathExpressions::Mul::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    // Same as above
    long double res = 1;
    std::vector<long double> params;
    EvaluateChildren(node, params, env);

    if (params.size() < 2) throw UnexpectedSubexpressionCount(this, params.size(), 2);

    for (long double param : params)
        res *= param;

    return res;
}

TOKEN_CONSTR_IMPL(Div, BinaryOp);

size_t MathExpressions::Div::GetPriority() const
{
    return 2;
}

long double MathExpressions::Div::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    // Same as above
    std::vector<long double> params;
    EvaluateChildren(node, params, env);

    if (params.size() < 2) throw UnexpectedSubexpressionCount(this, params.size(), 2);

    long double res = params[0];
    for (size_t i = 1; i < params.size(); i++)
    {
        if (params[i] == 0) throw DivisionByZero(this);
        res /= params[i];
    }

    return res;
}

TOKEN_CONSTR_IMPL(Pow, BinaryOp);

size_t MathExpressions::Pow::GetPriority() const
{
    return 3;
}

long double MathExpressions::Pow::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 2);

    // Cannot get root from negative numbers
    if (params[1] < 1.0 && params[0] < 0.0) throw NegativeNumberRoot(this);

    return powl(params[0], params[1]);
}

TOKEN_CONSTR_IMPL(Pair, Token);

void MathExpressions::Pair::FindNextToken(
    View<std::vector<Parser::TokenPtr>> token_range,
    std::vector<Parser::TokenPtr>::const_iterator& out_token
) const {
    // Finds this token's matching token in an array
    FindMatchingToken(token_range, out_token);

    // Moves the cursor past it's pair
    out_token++;
}

void MathExpressions::Pair::SplitPoints(
    View<std::vector<Parser::TokenPtr>> partition_range,
    std::vector<Parser::TokenPtr>::const_iterator cur_token,
    std::vector<View<std::vector<Parser::TokenPtr>>>& out_partition
) const {
    out_partition.clear();

    std::vector<Parser::TokenPtr>::const_iterator past_bracket = cur_token + 1;
    std::vector<Parser::TokenPtr>::const_iterator closing_bracket = cur_token;
    FindMatchingToken(partition_range, closing_bracket);

    // This token's subexpression is everything sandwitched between it and it's pair
    out_partition.push_back({ partition_range.Source, past_bracket, closing_bracket });
}

void MathExpressions::Pair::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    SourcedToken::Stringify(tree, cur_node, out_expression);

    for (Tree<Parser::TokenPtr>::NodePtr child_node : cur_node.Children)
        child_node->Value->Stringify(tree, *child_node, out_expression);
}

void MathExpressions::Pair::Backpatch(
    std::vector<Parser::TokenPtr>& tokens,
    std::vector<Parser::TokenPtr>::iterator cur_token
) {
    std::vector<Parser::TokenPtr>::const_iterator pair = cur_token;
    FindMatchingToken(View<std::vector<Parser::TokenPtr>>(&tokens, tokens.cbegin(), tokens.cend()), pair);

    PairCache.insert(std::make_pair(&tokens, pair));
}

MathExpressions::DistinctPair::DistinctPair(
    View<std::string> source_range,
    bool variant
) : Variant(variant), Pair(source_range)
{}

void MathExpressions::DistinctPair::FindMatchingToken(
    View<std::vector<Parser::TokenPtr>> token_range, 
    std::vector<Parser::TokenPtr>::const_iterator& out_token
) const {
    const PairCacheMap::const_iterator it = PairCache.find(token_range.Source);
    if (it != PairCache.cend())
    {
        out_token = it->second;
        return;
    }

    out_token++;

    // Look for tokens that could potentially be a pair
    for (; out_token != token_range.End; (*out_token)->FindNextToken(token_range, out_token))
    {
        auto pair = std::dynamic_pointer_cast<const MathExpressions::DistinctPair>(*out_token);
        if (!pair || !IsMatchingToken(pair.get())) continue;

        // If it's the second token in the pair, halt the function
        // At this point the pointer is already at that token, so
        // nothing extra has to be done
        if (pair->Variant) return;
    }

    throw NoMatchingToken(this);
}

MathExpressions::Bracket::Bracket(
    View<std::string> source_range,
    bool closing
) : DistinctPair(source_range, closing) {};

void MathExpressions::Bracket::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    Pair::Stringify(tree, cur_node, out_expression);

    out_expression.push_back(')');
}

size_t MathExpressions::Bracket::GetPriority() const
{
    // Bracket is also has the most priority
    return std::numeric_limits<size_t>::max();
}

bool MathExpressions::Bracket::IsMatchingToken(const Pair* token) const
{
    // Match with other tokens
    return static_cast<bool>(dynamic_cast<const Bracket*>(token));
}

long double MathExpressions::Bracket::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    // Simply pipe up whatever is inbetween
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return params[0];
}

TOKEN_CONSTR_IMPL(IndistinctPair, Pair);

void MathExpressions::IndistinctPair::FindMatchingToken(
    View<std::vector<Parser::TokenPtr>> token_range,
    std::vector<Parser::TokenPtr>::const_iterator& out_token
) const {
    out_token++;

    for (; out_token != token_range.End; (*out_token)->FindNextToken(token_range, out_token)) 
    {
        // Unlike brackets, there's no way we can differentiate between closing and opening modulo brackets
        // The closing bracket can be determined in multiple ways, but, currently it just matches first other found
        auto pair = std::dynamic_pointer_cast<const MathExpressions::IndistinctPair>(*out_token);
        if (pair && IsMatchingToken(pair.get())) return;
    }

    throw NoMatchingToken(this);
}

TOKEN_CONSTR_IMPL(ModBracket, IndistinctPair);

void MathExpressions::ModBracket::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    Pair::Stringify(tree, cur_node, out_expression);

    out_expression.push_back('|');
}

size_t MathExpressions::ModBracket::GetPriority() const
{
    return std::numeric_limits<size_t>::max();
}

bool MathExpressions::ModBracket::IsMatchingToken(const Pair* Pair) const
{
    return static_cast<bool>(dynamic_cast<const ModBracket*>(Pair));
}

long double MathExpressions::ModBracket::Evaluate(
    const Tree<Parser::TokenPtr>::NodePtr& node, 
    const MathExpressions::Environment& env
) const {
    // Return the absolute value of the value of whatever subexpression brackets have
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return fabsl(params[0]);
}

MathExpressions::Function::Function(
    View<std::string> source_range
) : DistinctPair(source_range, false) {};

void MathExpressions::Function::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    Pair::Stringify(tree, cur_node, out_expression);

    out_expression.push_back(')');
}

size_t MathExpressions::Function::GetPriority() const
{
    return 4;
}

bool MathExpressions::Function::IsMatchingToken(const Pair* pair) const
{
    // The easy route of just matching functions with closing brackets, because,
    // they do end with a closing bracket
    return static_cast<bool>(dynamic_cast<const MathExpressions::Bracket*>(pair));
}

TOKEN_CONSTR_IMPL(LogarithmE, Function);

long double MathExpressions::LogarithmE::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return logl(params[0]);
}

TOKEN_CONSTR_IMPL(Logarithm2, Function);

long double MathExpressions::Logarithm2::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return log2l(params[0]);
}

TOKEN_CONSTR_IMPL(Logarithm10, Function);

long double MathExpressions::Logarithm10::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return log10l(params[0]);
}

TOKEN_CONSTR_IMPL(ArgumentedFunction, Function);

void MathExpressions::ArgumentedFunction::SplitPoints(
    View<std::vector<Parser::TokenPtr>> expression_range,
    std::vector<Parser::TokenPtr>::const_iterator cur_token,
    std::vector<View<std::vector<Parser::TokenPtr>>>& partitioned_range
) const {
    Pair::SplitPoints(expression_range, cur_token, partitioned_range);

    const View<std::vector<Parser::TokenPtr>>& found_range = partitioned_range[0];

    std::vector<std::vector<Parser::TokenPtr>::const_iterator> found_separators;

    // Finds all the separators between brackets of a function
    for (
        std::vector<Parser::TokenPtr>::const_iterator it = found_range.Start;
        it != found_range.End; (*it)->FindNextToken(found_range, it)
    ) {
        if (!std::dynamic_pointer_cast<const MathExpressions::ParamSeparator>(*it)) continue;

        found_separators.push_back(it);
    }

    if (found_separators.empty()) return;

    std::vector<View<std::vector<Parser::TokenPtr>>> new_partitioned_range;

    // Makes expressions between brackets and each separator subexpressions of the function
    std::vector<Parser::TokenPtr>::const_iterator prev_separator = found_range.Start;
    for (std::vector<Parser::TokenPtr>::const_iterator separator : found_separators)
    {
        new_partitioned_range.push_back({ expression_range.Source, prev_separator, separator });
        prev_separator = separator + 1;
    }

    new_partitioned_range.push_back({ expression_range.Source, prev_separator, found_range.End });

    partitioned_range = new_partitioned_range;
}

void MathExpressions::ArgumentedFunction::Stringify(
    const Tree<Parser::TokenPtr>& tree,
    const Tree<Parser::TokenPtr>::Node& cur_node,
    std::string& out_expression
) const {
    out_expression += std::string(Source.Start, Source.End);

    cur_node.Children[0]->Value->Stringify(tree, *cur_node.Children[0], out_expression);

    for (
        std::vector<Tree<Parser::TokenPtr>::NodePtr>::const_iterator it = cur_node.Children.cbegin() + 1;
        it != cur_node.Children.cend(); ++it
    ) {
        out_expression.append(", ");
        (*it)->Value->Stringify(tree, **it, out_expression);
    }
}

TOKEN_CONSTR_IMPL(Logarithm, ArgumentedFunction);

long double MathExpressions::Logarithm::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 2);

    return logl(params[0]) / logl(params[1]);
}

TOKEN_CONSTR_IMPL(ExponentFunc, Function);

long double MathExpressions::ExponentFunc::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return expl(params[0]);
}

TOKEN_CONSTR_IMPL(SquareRoot, Function);

long double MathExpressions::SquareRoot::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    if (params[0] < 0) throw NegativeNumberRoot(this);

    return sqrtl(params[0]);
}

TOKEN_CONSTR_IMPL(Sign, Function);

long double MathExpressions::Sign::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return (params[0] == 0) ? 0 : ((params[0] > 0) ? 1 : -1);
}

TOKEN_CONSTR_IMPL(Sine, Function);

long double MathExpressions::Sine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return sinl(params[0]);
}

TOKEN_CONSTR_IMPL(Cosine, Function);

long double MathExpressions::Cosine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return cosl(params[0]);
}

TOKEN_CONSTR_IMPL(Tangent, Function);

long double MathExpressions::Tangent::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return tanl(params[0]);
}

TOKEN_CONSTR_IMPL(Cotangent, Function);

long double MathExpressions::Cotangent::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return 1 / tanl(params[0]);
}

TOKEN_CONSTR_IMPL(Arcsine, Function);

long double MathExpressions::Arcsine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return asinl(params[0]);
}

TOKEN_CONSTR_IMPL(Arccosine, Function);

long double MathExpressions::Arccosine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return acosl(params[0]);
}

TOKEN_CONSTR_IMPL(Arctangent, Function);

long double MathExpressions::Arctangent::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return atanl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicSine, Function);

long double MathExpressions::HyperbolicSine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return sinhl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicCosine, Function);

long double MathExpressions::HyperbolicCosine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return coshl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicTangent, Function);

long double MathExpressions::HyperbolicTangent::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return tanhl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicArcsine, Function);

long double MathExpressions::HyperbolicArcsine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return asinhl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicArccosine, Function);

long double MathExpressions::HyperbolicArccosine::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return acoshl(params[0]);
}

TOKEN_CONSTR_IMPL(HyperbolicArctangent, Function);

long double MathExpressions::HyperbolicArctangent::Evaluate(const Tree<Parser::TokenPtr>::NodePtr& node, const MathExpressions::Environment& env) const
{
    std::vector<long double> params;
    EvaluateChildren(node, params, env, 1);

    return atanhl(params[0]);
}

// Set of factories fed to 'Parse' method of a parser
// Matches a number
static Parser::TokenPtr MET_NumberFactory(const std::string& in_expr, size_t& cursor)
{
    // Whether number is fraction or a whole number
    bool num_dec = false;
    size_t original_cursor = cursor;
    // Character on current iteration
    char ch;

    do
    {
        ch = in_expr[cursor];

        // Number should be fractional if we encounter a dot
        if (ch == '.')
        {
            // If we have already registered number as a fraction, we're meeting a duplicate of dot
            if (num_dec) throw IncorrectlyFormedNumber(cursor);
            else num_dec = true;
        }

        cursor++;
    } while ((std::isdigit(ch) || ch == '.') && cursor <= in_expr.size());

    cursor--;

    // If there were no digits, bail
    if (cursor == original_cursor) return Parser::TokenPtr();

    return std::make_shared<MathExpressions::Number>(
        View<std::string>(&in_expr, in_expr.cbegin() + original_cursor, in_expr.cbegin() + cursor)
    );
}

// Matches templated token from a single character
template<typename T>
static Parser::TokenPtr TokenFromCharacter(
    const std::string& in_expr, size_t& cursor,
    char ch
) {
    // If we meet specified character, it's a match
    if (in_expr[cursor] == ch)
    {
        std::string::const_iterator start = in_expr.cbegin() + cursor;
        cursor++;
        std::string::const_iterator end = in_expr.cbegin() + cursor;
        return std::make_shared<T>(View<std::string>(&in_expr, start, end));
    }

    return Parser::TokenPtr();
}

static Parser::TokenPtr MET_ExponentConstFactory(const std::string& in_expr, size_t& cursor)
{
    // Match 'e' as Euler's number constant
    return TokenFromCharacter<MathExpressions::ExponentConst>(in_expr, cursor, 'e');
}

// Matches templated token from a substring
template<typename T>
static Parser::TokenPtr TokenFromString(
    const std::string& in_expr, size_t& cursor,
    const std::string& from
) {
    // If the remaining string is shorter than the substring, bail
    if (cursor + from.size() > in_expr.size()) return Parser::TokenPtr();

    size_t original_cursor = cursor;

    // Check if expression substring and provided substring are equal
    bool is_equal = true;
    for (char ch : from)
    {
        if (in_expr[cursor] != ch)
        {
            is_equal = false;
            break;
        }

        cursor++;
    }

    return (is_equal) ? 
        std::make_shared<T>(View<std::string>(&in_expr, in_expr.cbegin() + original_cursor, in_expr.cbegin() + cursor))
        : Parser::TokenPtr();
}

static Parser::TokenPtr MET_PythagoreanFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string const_name = "pi";

    return TokenFromString<MathExpressions::Pythagorean>(in_expr, cursor, const_name);
}

// Matches templated token from either of provided substrings
template<typename T>
static Parser::TokenPtr TokenFromEitherStrings(
    const std::string& in_expr, size_t& cursor,
    const std::vector<std::string>& from_strings
) {
    for (const std::string& from : from_strings)
        if (Parser::TokenPtr token = TokenFromString<T>(in_expr, cursor, from)) return token;

    return Parser::TokenPtr();
}

static Parser::TokenPtr MET_AddFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::Add>(in_expr, cursor, '+');
}

static Parser::TokenPtr MET_SubFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::Sub>(in_expr, cursor, '-');
}

static Parser::TokenPtr MET_MulFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::Mul>(in_expr, cursor, '*');
}

static Parser::TokenPtr MET_DivFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::Div>(in_expr, cursor, '/');
}

static Parser::TokenPtr MET_PowFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::Pow>(in_expr, cursor, '^');
}

static Parser::TokenPtr MET_BracketFactory(const std::string& in_expr, size_t& cursor)
{
    std::string::const_iterator start = in_expr.cbegin() + cursor, end = in_expr.cbegin() + cursor + 1;
    if (in_expr[cursor] == '(')
    {
        cursor++;
        // If bracket is opening, the 'Variant' on Bracket instance is set to false
        return std::make_shared<MathExpressions::Bracket>(View<std::string>(&in_expr, start, end), false);
    }

    if (in_expr[cursor] == ')')
    {
        cursor++;
        // ...otherwise it's set to true
        return std::make_shared<MathExpressions::Bracket>(View<std::string>(&in_expr, start, end), true);
    }

    return Parser::TokenPtr();
}

static Parser::TokenPtr MET_ModBracketFactory(const std::string& in_expr, size_t& cursor)
{
    return TokenFromCharacter<MathExpressions::ModBracket>(in_expr, cursor, '|');
}

static Parser::TokenPtr MET_LogarithmEFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "ln(";

    return TokenFromString<MathExpressions::LogarithmE>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_Logarithm2Factory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "log2(";

    return TokenFromString<MathExpressions::Logarithm2>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_Logarithm10Factory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "log10(";

    return TokenFromString<MathExpressions::Logarithm10>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_LogarithmFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "log(";

    return TokenFromString<MathExpressions::Logarithm>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_ExponentFuncFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "exp(";

    return TokenFromString<MathExpressions::ExponentFunc>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_SquareRootFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "sqrt(";

    return TokenFromString<MathExpressions::SquareRoot>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_SignFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "sign(";

    return TokenFromString<MathExpressions::Sign>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_SineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "sin(";

    return TokenFromString<MathExpressions::Sine>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_CosineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "cos(";

    return TokenFromString<MathExpressions::Cosine>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_TangentFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "tg(", "tan(" };

    return TokenFromEitherStrings<MathExpressions::Tangent>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_CotangentFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "ctg(", "ctan(" };

    return TokenFromEitherStrings<MathExpressions::Cotangent>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_ArcsineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "asin(", "arcsin(" };

    return TokenFromEitherStrings<MathExpressions::Arcsine>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_ArccosineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "acos(", "arccos(" };

    return TokenFromEitherStrings<MathExpressions::Arccosine>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_ArctangentFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "atg(", "atan(", "arctg(", "arctan(" };

    return TokenFromEitherStrings<MathExpressions::Arctangent>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_HyperbolicSineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "sinh(";

    return TokenFromString<MathExpressions::HyperbolicSine>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_HyperbolicCosineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::string func_name = "cosh(";

    return TokenFromString<MathExpressions::HyperbolicCosine>(in_expr, cursor, func_name);
}

static Parser::TokenPtr MET_HyperbolicTangentFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "tgh(", "tanh(" };

    return TokenFromEitherStrings<MathExpressions::HyperbolicTangent>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_HyperbolicArcsineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "asinh(", "arcsinh(" };

    return TokenFromEitherStrings<MathExpressions::HyperbolicArcsine>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_HyperbolicArccosineFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "acosh(", "arccosh(" };

    return TokenFromEitherStrings<MathExpressions::HyperbolicArccosine>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_HyperbolicArctangentFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<std::string> func_aliases = { "atgh(", "atanh(", "arctgh(", "arctanh(" };

    return TokenFromEitherStrings<MathExpressions::HyperbolicTangent>(in_expr, cursor, func_aliases);
}

static Parser::TokenPtr MET_VariableFactory(const std::string& in_expr, size_t& cursor)
{
    std::string::const_iterator start = in_expr.cbegin() + cursor, end = start;
    for (; end != in_expr.cend() && std::isalpha(*end); ++end, ++cursor);

    return (start != end) ? 
        std::make_shared<MathExpressions::Variable>(View<std::string>(&in_expr, start, end)) :
        Parser::TokenPtr();
}


TOKEN_CONSTR_IMPL(ParamSeparator, SourcedToken);

bool MathExpressions::ParamSeparator::IsPrecedent(const Parser::IToken*) const
{
    // Because separators should only be sandwitched between brackets in functions
    // precedency does not matter
    return true;
}

void MathExpressions::ParamSeparator::FindNextToken(
    View<std::vector<Parser::TokenPtr>>,
    std::vector<Parser::TokenPtr>::const_iterator& token
) const {
    token++;
}

void MathExpressions::ParamSeparator::SplitPoints(
    View<std::vector<Parser::TokenPtr>>,
    std::vector<Parser::TokenPtr>::const_iterator,
    std::vector<View<std::vector<Parser::TokenPtr>>>&
) const {
    // Even considering whether separators should have subexpressions
    // hints that expression itself is invalid
    throw UnexpectedSeparator(this);
}

// Matches templated token from any of the provided characters
template<typename T>
static Parser::TokenPtr TokenFromEitherChars(
    const std::string& in_expr, size_t& cursor,
    const std::vector<char>& from_chars
) {
    for (char from : from_chars)
        if (const Parser::TokenPtr& token = TokenFromCharacter<T>(in_expr, cursor, from)) return token;

    return Parser::TokenPtr();
}

static Parser::TokenPtr MET_SeparatorFactory(const std::string& in_expr, size_t& cursor)
{
    static const std::vector<char> separators = { ',', ';' };
    return TokenFromEitherChars<MathExpressions::ParamSeparator>(in_expr, cursor, separators);
}

// Consumes whitespace (i.e. sets the cursor at the end of any encountered whitespace in the string)
static Parser::TokenPtr MET_WhitespaceFactory(const std::string& in_expr, size_t& cursor)
{
    for (; std::isblank(in_expr[cursor]); cursor++) {};

    return Parser::TokenPtr();
}

const std::vector<Parser::TokenFactory>& MathExpressions::GetTokenFactories()
{
    /* Pre - generated vector with all the factories packed into it
    Notice the order. Because parser just iterates over this vector linearly,
    order of factories determines their priority. For instance, nothing stopping
    'pi' from being just two consequtive variables, but because MET_PythagoreanFactory
    is ordered before MET_VariableFactory, it always matches as a pi constant
    */
    static const std::vector<Parser::TokenFactory> ME_Factories = 
    { 
        MET_WhitespaceFactory, 

        MET_BracketFactory, MET_ModBracketFactory,

        MET_AddFactory, MET_SubFactory,
        MET_MulFactory, MET_DivFactory,
        MET_PowFactory,

        MET_LogarithmEFactory, MET_Logarithm2Factory,
        MET_Logarithm10Factory, MET_LogarithmFactory,
        MET_ExponentFuncFactory,
        MET_SquareRootFactory, MET_SignFactory,
        MET_SineFactory, MET_CosineFactory,
        MET_TangentFactory, MET_CotangentFactory,
        MET_ArcsineFactory, MET_ArccosineFactory,
        MET_ArctangentFactory,
        MET_HyperbolicSineFactory, MET_HyperbolicCosineFactory,
        MET_HyperbolicTangentFactory,
        MET_HyperbolicArcsineFactory, MET_HyperbolicArccosineFactory,
        MET_HyperbolicArctangentFactory,

        MET_SeparatorFactory,

        MET_NumberFactory, MET_PythagoreanFactory,
        MET_ExponentConstFactory, MET_VariableFactory
    };

    return ME_Factories;
}

long double MathExpressions::Evaluate(
    const std::string& expression, 
    const Environment& env, 
    std::vector<Parser::TokenPtr>& out_tokens,
    Tree<Parser::TokenPtr>& out_ast)
{
    if (expression.empty()) throw std::runtime_error("Empty expression provided");

    Parser::Engine parser;

    parser.Tokenize(MathExpressions::GetTokenFactories(), expression, out_tokens);
    parser.Parse(out_tokens, out_ast);

    const Parser::TokenPtr token = out_ast.Root->Value;
    auto& math_token = std::dynamic_pointer_cast<MathExpressions::Token>(token);
    if (!math_token) throw std::runtime_error("Parser did not return correct token type ('MathExpression::Token')");

    return math_token->Evaluate(out_ast.Root, env);
}
