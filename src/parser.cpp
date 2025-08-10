#include "parser.hpp"

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>

#include "ast.hpp"
#include "lexer.hpp"

namespace ks
{

static std::unique_ptr<ExprAST> LogError(const std::string_view str)
{
    std::cerr << std::format("Error: {}\n", str);
    return nullptr;
}

static std::unique_ptr<FunctionAST> LogErrorF(const std::string_view str)
{
    LogError(str);
    return nullptr;
}

static std::unique_ptr<PrototypeAST> LogErrorP(const std::string_view str)
{
    LogError(str);
    return nullptr;
}

Token Parser::get_next_token()
{
    return this->current_token = this->lexer.get_token();
}

std::unique_ptr<PrototypeAST> Parser::gen_annon_expr()
{
    const auto id = this->annon++;
    auto ss = std::stringstream();
    ss << "__annon_expr" << id;
    return std::make_unique<PrototypeAST>(ss.str(), std::vector<std::string>());
}

std::optional<std::vector<std::unique_ptr<ExprAST>>> Parser::parse_args()
{
    auto args = std::vector<std::unique_ptr<ExprAST>>();
    while (this->current_token.ty != TokenType::RIGHT_PAREN)
    {
        if (auto arg = parse_expression())
        {
            args.push_back(std::move(arg));
        }
        else
        {
            return std::nullopt;
        }
    }
    return args;
}

/// call-expr
///     ::= expression expression*
std::unique_ptr<ExprAST> Parser::parse_call_expression()
{
    if (this->current_token.ty != TokenType::IDENTIFIER)
    {
        return LogError(std::format("Expected identifier, found {}", this->current_token));
    }
    const auto callee = this->current_token.str;

    // Eat callee.
    this->get_next_token();

    auto args = parse_args();
    if (args.has_value())
    {
        return std::make_unique<CallExprAST>(std::move(callee), std::move(args.value()));
    }
    else
    {
        return nullptr;
    }
}

/// expr
///     ::= identifier
///     ::= '(' call-expr ')'
std::unique_ptr<ExprAST> Parser::parse_expression()
{
    if (this->current_token.ty == TokenType::IDENTIFIER)
    {
        auto expr = std::make_unique<VariableExprAST>(this->current_token.str);
        // Eat the identifier.
        this->get_next_token();
        return expr;
    }
    else if (this->current_token.ty == TokenType::LEFT_PAREN)
    {
        // Eat the '('.
        this->get_next_token();
        auto expr = this->parse_call_expression();
        // Eat the ')'.
        this->get_next_token();
        return expr;
    }
    else
    {
        return LogError(std::format("Expected '(' or identifier, found {}", this->current_token));
    }
}

/// prototype
///     ::= '(' identifier identifier* ')'
std::unique_ptr<PrototypeAST> Parser::parse_prototype()
{
    if (this->current_token.ty != TokenType::LEFT_PAREN)
    {
        return LogErrorP(std::format("Expected '(', found: {}", this->current_token));
    }
    const auto name = this->get_next_token();
    if (name.ty != TokenType::IDENTIFIER)
    {
        return LogErrorP(std::format("Expected identifier, found: {}", name));
    }
    const auto name_str = this->current_token.str;
    auto args = std::vector<std::string>();
    while (true)
    {
        auto arg = this->get_next_token();
        if (arg.ty == TokenType::IDENTIFIER)
        {
            args.push_back(this->current_token.str);
        }
        else if (arg.ty == TokenType::RIGHT_PAREN)
        {
            break;
        }
        else
        {
            return LogErrorP(std::format("Expected identifier or ')', found {}", arg));
        }
    }
    // Eat the ')'.
    this->get_next_token();
    return std::make_unique<PrototypeAST>(std::move(name_str), std::move(args));
}

/// define_statement
///     ::= define prototype expression
std::unique_ptr<FunctionAST> Parser::parse_define()
{
    if (this->current_token.ty != TokenType::DEF)
    {
        return LogErrorF(std::format("Expected 'define', found :{}", this->current_token));
    }

    // Eat the 'define'
    this->get_next_token();
    auto proto = this->parse_prototype();
    if (proto == nullptr)
    {
        return nullptr;
    }
    auto expr = this->parse_expression();
    if (expr == nullptr)
    {
        return nullptr;
    }

    auto def = std::make_unique<FunctionAST>(std::move(proto), std::move(expr));
    std::cout << def->to_string() << std::endl;
    return def;
}

/// extern
///     ::= extern prototype
std::unique_ptr<PrototypeAST> Parser::parse_extern()
{
    if (this->current_token.ty != TokenType::EXTERN)
    {
        return LogErrorP(std::format("Expected extern, found {}", this->current_token));
    }
    // Eat the 'extern'.
    this->get_next_token();

    auto proto = this->parse_prototype();
    std::cout << proto->to_string() << std::endl;
    return proto;
}

std::unique_ptr<FunctionAST> Parser::parse_top_level_expr()
{
    if (auto e = this->parse_call_expression())
    {
        auto proto = this->gen_annon_expr();
        auto expr = std::make_unique<FunctionAST>(std::move(proto), std::move(e), true);
        std::cout << expr->to_string() << std::endl;
        return expr;
    }
    else
    {
        return nullptr;
    }
}

std::unique_ptr<FunctionAST> Parser::parse_top_level_identifier()
{
    if (this->current_token.ty != TokenType::IDENTIFIER)
    {
        return LogErrorF(std::format("Expected identifier, found: {}", this->current_token));
    }
    const auto name = this->current_token.str;

    auto proto = this->gen_annon_expr();
    auto expr = std::make_unique<FunctionAST>(std::move(proto), std::make_unique<VariableExprAST>(name), true);
    std::cout << expr->to_string() << std::endl;
    return expr;
}

std::optional<Parser::ParseResult> Parser::parse_top_level()
{
    auto tok = this->get_next_token();
    if (tok.ty == TokenType::END_OF_FILE)
    {
        return std::nullopt;
    }
    else if (tok.ty == TokenType::IDENTIFIER)
    {
        auto expr = this->parse_top_level_identifier();
        if (expr == nullptr)
        {
            return std::nullopt;
        }
        else
        {
            return expr;
        }
    }
    else if (tok.ty == TokenType::LEFT_PAREN)
    {
        tok = this->get_next_token();
        ParseResult result;
        if (tok.ty == TokenType::IDENTIFIER || tok.ty == TokenType::LEFT_PAREN)
        {
            auto expr = this->parse_top_level_expr();
            if (expr == nullptr)
            {
                return std::nullopt;
            }
            result = std::move(expr);
        }
        else if (tok.ty == TokenType::DEF)
        {
            auto def = this->parse_define();
            if (def == nullptr)
            {
                return std::nullopt;
            }
            result = std::move(def);
        }
        else if (tok.ty == TokenType::EXTERN)
        {
            auto ext = this->parse_extern();
            if (ext == nullptr)
            {
                return std::nullopt;
            }
            result = std::move(ext);
        }
        else
        {
            LogError(std::format("at parse_top_level(): expected identifier, define "
                                 "or extern, found {}",
                                 this->current_token));
            return std::nullopt;
        }
        if (this->current_token.ty != TokenType::RIGHT_PAREN)
        {
            LogError(std::format("at parse_top_level(): expected ')', found {}", this->current_token));
            return std::nullopt;
        }

        return result;
    }
    else
    {
        return std::nullopt;
    }
}

} // namespace ks
