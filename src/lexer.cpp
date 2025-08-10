#include "lexer.hpp"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace ks
{

Token Lexer::get_token()
{
    while ((std::isspace(this->last_char) || this->last_char == 0) && this->last_char != EOF)
    {
        this->last_char = this->getchar();
    }
    if (this->last_char == EOF)
    {
        return Token(TokenType::END_OF_FILE);
    }
    else if (this->last_char == ';')
    {
        do
        {
            this->last_char = this->getchar();
        } while (this->last_char != EOF && this->last_char != '\n' && this->last_char != '\r');

        return get_token();
    }
    else if (this->last_char == '(')
    {
        this->last_char = this->getchar();
        return Token(TokenType::LEFT_PAREN);
    }
    else if (this->last_char == ')')
    {
        this->last_char = this->getchar();
        return Token(TokenType::RIGHT_PAREN);
    }
    else
    { // Identifiers
        auto identifier_ss = std::stringstream();
        identifier_ss << this->last_char;
        while (true)
        {
            this->last_char = this->getchar();
            if (std::isspace(this->last_char) || this->last_char == EOF)
            {
                break;
            }
            else if (this->last_char == '(' || this->last_char == ')')
            {
                break;
            }
            else
            {
                identifier_ss << this->last_char;
            }
        }

        if (identifier_ss.str() == "define")
        {
            return Token(TokenType::DEF);
        }
        else if (identifier_ss.str() == "extern")
        {
            return Token(TokenType::EXTERN);
        }
        else
        {
            return Token(TokenType::IDENTIFIER, identifier_ss.str());
        }
    } // end identifiers
    assert(false); // unreachable
    std::exit(-1);
}
} // namespace ks
