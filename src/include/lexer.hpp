#pragma once

#include <cassert>
#include <format>
#include <iostream>
#include <istream>
#include <string>

namespace ks {

enum TokenType {
    END_OF_FILE,
    DEF,
    EXTERN,
    IDENTIFIER,
    LEFT_PAREN,
    RIGHT_PAREN,
};

struct Token {
    std::string str;
    TokenType ty;

    Token(TokenType _ty, std::string _str = "") : str(_str), ty(_ty) {}
};

class Lexer {
 public:
    explicit Lexer(std::istream& _is) : is(_is) {
        this->is >> std::noskipws;
    }
    Token get_token();
 private:
    std::istream& is;
    char last_char = ' ';

    char getchar() {
        if (this->is.eof()) {
            return EOF;
        }
        char c;
        this->is >> c;
        return c;
    }
};

}  // namespace ks

template <>
struct std::formatter<ks::Token> : std::formatter<std::string> {
    auto format(ks::Token t, format_context& ctx) const {
        const auto str = [t](){
            #define TOKEN_TO_STRING(TY) \
                case ks::TokenType::TY: return std::string(#TY);
            switch (t.ty) {
                TOKEN_TO_STRING(END_OF_FILE)
                TOKEN_TO_STRING(DEF)
                TOKEN_TO_STRING(EXTERN)
                TOKEN_TO_STRING(LEFT_PAREN)
                TOKEN_TO_STRING(RIGHT_PAREN)
                case ks::TokenType::IDENTIFIER:
                    return std::format("IDENTIFIER({})", t.str);
            }
            #undef TOKEN_TO_STRING
            assert(false);
        }();
        return formatter<std::string>::format(
            str, ctx
        );
    }
};
