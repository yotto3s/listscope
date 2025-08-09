#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "ast.hpp"
#include "lexer.hpp"

namespace ks {

class Parser {
 public:
    using ParseResult = std::variant<std::unique_ptr<PrototypeAST>,
                                     std::unique_ptr<FunctionAST>>;
    Parser(Lexer&& _lexer) : lexer(std::move(_lexer)) {}
    std::optional<ParseResult> parse_top_level();
 private:
    Token current_token = Token(TokenType::END_OF_FILE);
    Lexer lexer;
    std::size_t annon = 0u;

    Token get_next_token();
    std::unique_ptr<PrototypeAST> gen_annon_expr();
    std::unique_ptr<ExprAST> parse_expression();
    std::optional<std::vector<std::unique_ptr<ExprAST>>> parse_args();
    std::unique_ptr<ExprAST> parse_call_expression();
    std::unique_ptr<PrototypeAST> parse_prototype();
    std::unique_ptr<FunctionAST> parse_define();
    std::unique_ptr<PrototypeAST> parse_extern();
    std::unique_ptr<FunctionAST> parse_top_level_expr();
    std::unique_ptr<FunctionAST> parse_top_level_identifier();
};
}  // namespace ks
