#include "lexer.hpp"

#include <cctype>

// Nome legível de cada tipo de token (para erros e depuração).
std::string tokenTypeName(TokenType type)
{
    switch (type)
    {
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::COLON:       return "COLON";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::TYPE_VALUE:  return "TYPE_VALUE";
        case TokenType::ASSIGN:      return "ASSIGN";
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::STRING:      return "STRING";
        case TokenType::LEFT_PAREN:  return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::EOFILE:      return "EOFILE";
        case TokenType::UNKNOWN:     return "UNKNOWN";
    }
    return "UNKNOWN";
}

Lexer::Lexer(const std::string& source)
    : source(source)
{
}

// --- Navegação pelo texto ---------------------------------------------------

bool Lexer::atEnd() const
{
    return position >= source.size();
}

char Lexer::current() const
{
    return atEnd() ? '\0' : source[position];
}

char Lexer::peekNext() const
{
    return (position + 1 < source.size()) ? source[position + 1] : '\0';
}

void Lexer::advance()
{
    if (atEnd()) return;

    if (source[position] == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    position++;
}

// --- Leitura de tokens compostos --------------------------------------------

// Uma palavra é uma letra ou '_' seguida de letras/dígitos/'_'.
// Pode ser uma palavra-chave de tipo (int, string, auto) ou um identificador.
Token Lexer::readWord()
{
    const int startLine = line;
    const int startCol  = column;

    std::string word;
    while (!atEnd() && (std::isalnum(current()) || current() == '_'))
    {
        word += current();
        advance();
    }

    // Classifica a palavra: palavra-chave de tipo, literal booleano ou nome.
    TokenType type;
    if (word == "int" || word == "string" || word == "bool" || word == "auto")
        type = TokenType::TYPE_VALUE;
    else if (word == "true" || word == "false")
        type = TokenType::BOOLEAN;
    else
        type = TokenType::IDENTIFIER;

    return {type, word, startLine, startCol};
}

Token Lexer::readNumber()
{
    const int startLine = line;
    const int startCol  = column;

    std::string number;
    while (!atEnd() && std::isdigit(current()))
    {
        number += current();
        advance();
    }

    return {TokenType::NUMBER, number, startLine, startCol};
}

// Lê uma string entre aspas duplas. Suporta escapes básicos (\n, \t, \", \\).
Token Lexer::readString()
{
    const int startLine = line;
    const int startCol  = column;

    advance(); // consome a aspa de abertura

    std::string text;
    while (!atEnd() && current() != '"')
    {
        if (current() == '\\' && peekNext() != '\0')
        {
            advance(); // consome a '\'
            switch (current())
            {
                case 'n': text += '\n'; break;
                case 't': text += '\t'; break;
                case '"': text += '"';  break;
                case '\\': text += '\\'; break;
                default:  text += current(); break;
            }
            advance();
        }
        else
        {
            text += current();
            advance();
        }
    }

    advance(); // consome a aspa de fechamento

    return {TokenType::STRING, text, startLine, startCol};
}

// --- Laço principal ---------------------------------------------------------

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (!atEnd())
    {
        const char c = current();

        // Espaços em branco: apenas pulamos.
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            advance();
            continue;
        }

        // Palavras (identificadores / tipos).
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            tokens.push_back(readWord());
            continue;
        }

        // Números.
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            tokens.push_back(readNumber());
            continue;
        }

        // Strings.
        if (c == '"')
        {
            tokens.push_back(readString());
            continue;
        }

        // Símbolos de um caractere.
        const int startLine = line;
        const int startCol  = column;
        TokenType type = TokenType::UNKNOWN;

        switch (c)
        {
            case ':': type = TokenType::COLON;       break;
            case ';': type = TokenType::SEMICOLON;   break;
            case ',': type = TokenType::COMMA;       break;
            case '=': type = TokenType::ASSIGN;      break;
            case '(': type = TokenType::LEFT_PAREN;  break;
            case ')': type = TokenType::RIGHT_PAREN; break;
            default:  type = TokenType::UNKNOWN;     break;
        }

        tokens.push_back({type, std::string(1, c), startLine, startCol});
        advance();
    }

    tokens.push_back({TokenType::EOFILE, "", line, column});
    return tokens;
}
