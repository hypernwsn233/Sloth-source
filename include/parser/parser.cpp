#include "parser.hpp"

#include <iostream>
#include <cstdlib>

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens))
{
}

// --- Navegação pelos tokens -------------------------------------------------

const Token &Parser::peek() const
{
    return tokens[position];
}

const Token &Parser::peekAt(size_t offset) const
{
    size_t index = position + offset;
    if (index >= tokens.size())
        index = tokens.size() - 1; // último token é sempre EOFILE
    return tokens[index];
}

bool Parser::check(TokenType type) const
{
    return peek().type == type;
}

bool Parser::atEnd() const
{
    return peek().type == TokenType::EOFILE;
}

void Parser::advance()
{
    if (!atEnd())
        position++;
}

Token Parser::consume(TokenType expected)
{
    if (check(expected))
    {
        Token token = peek();
        advance();
        return token;
    }

    error("esperava " + tokenTypeName(expected) +
          " mas encontrei " + tokenTypeName(peek().type) +
          " ('" + peek().value + "')");
}

void Parser::error(const std::string &message) const
{
    const Token &t = peek();
    std::cerr << "Erro de sintaxe (linha " << t.line << ", coluna " << t.column
              << "): " << message << std::endl;
    std::exit(1);
}

// --- Regras da gramática ----------------------------------------------------

std::unique_ptr<Program> Parser::parseProgram()
{
    auto program = std::make_unique<Program>();

    while (!atEnd())
    {
        program->addStatement(parseStatement());
    }

    return program;
}

// Decide se a próxima construção é uma declaração de variável ou uma chamada.
//   nome :  ...   -> declaração
//   nome (  ...   -> chamada
std::unique_ptr<ASTnode> Parser::parseStatement()
{
    if (check(TokenType::IDENTIFIER) && peekAt(1).type == TokenType::COLON)
    {
        return parseVariable();
    }

    if (check(TokenType::IDENTIFIER) && peekAt(1).type == TokenType::LEFT_PAREN)
    {
        auto call = parseFunctionCall();
        consume(TokenType::SEMICOLON);
        return call;
    }

    error("esperava uma declaracao de variavel ou chamada de funcao, "
          "mas encontrei " +
          tokenTypeName(peek().type) +
          " ('" + peek().value + "')");
}

// declaracao ::= IDENTIFIER ':' TYPE '=' expressao ';'
std::unique_ptr<ASTnode> Parser::parseVariable()
{
    Token name = consume(TokenType::IDENTIFIER);
    consume(TokenType::COLON);
    Token type = consume(TokenType::TYPE_VALUE);
    consume(TokenType::ASSIGN);

    std::unique_ptr<Expression> value = parseExpression();
    if (!value)
        error("esperava um valor na declaracao de '" + name.value + "'");

    consume(TokenType::SEMICOLON);

    // Resolve o tipo. "auto" é inferido a partir do valor inicial.
    std::string resolvedType = type.value;
    if (resolvedType == "auto")
    {
        if (dynamic_cast<NumberLiteral *>(value.get()))
            resolvedType = "int";
        else if (dynamic_cast<stringLiteral *>(value.get()))
            resolvedType = "string";
        else if (dynamic_cast<BooleanLiteral *>(value.get()))
            resolvedType = "bool";
    }

    return std::make_unique<VariableDeclaration>(
        name.value, resolvedType, std::move(value));
}

// chamada ::= IDENTIFIER '(' (expressao (',' expressao)*)? ')'
std::unique_ptr<Expression> Parser::parseFunctionCall()
{
    Token name = consume(TokenType::IDENTIFIER);
    consume(TokenType::LEFT_PAREN);

    std::vector<std::unique_ptr<Expression>> args;
    if (!check(TokenType::RIGHT_PAREN))
    {
        args.push_back(parseExpression());
        while (check(TokenType::COMMA))
        {
            advance();
            args.push_back(parseExpression());
        }
    }

    consume(TokenType::RIGHT_PAREN);

    return std::make_unique<FunctionCall>(name.value, std::move(args));
}

// expressao ::= NUMBER | STRING | IDENTIFIER | chamada
std::unique_ptr<Expression> Parser::parsePrimary()
{
    if (check(TokenType::NUMBER))
    {
        Token n = consume(TokenType::NUMBER);
        return std::make_unique<NumberLiteral>(std::stoi(n.value));
    }

    if (check(TokenType::STRING))
    {
        Token s = consume(TokenType::STRING);
        return std::make_unique<stringLiteral>(s.value);
    }

    if (check(TokenType::BOOLEAN))
    {
        Token b = consume(TokenType::BOOLEAN);
        bool value = (b.value == "true");
        return std::make_unique<BooleanLiteral>(value);
    }

    if (check(TokenType::IDENTIFIER))
    {
        // nome seguido de '(' é uma chamada de função aninhada.
        if (peekAt(1).type == TokenType::LEFT_PAREN)
            return parseFunctionCall();

        Token id = consume(TokenType::IDENTIFIER);
        return std::make_unique<Identifer>(id.value);
    }

    error("expressao invalida: token inesperado " +
          tokenTypeName(peek().type) + " ('" + peek().value + "')");
}

std::unique_ptr<Expression> Parser::parseExpression()
{
    auto left = parsePrimary();

    if (check(TokenType::GREATER) ||
        check(TokenType::LESS) ||
        check(TokenType::EQUAL_EQUAL) ||
        check(TokenType::NOT_EQUAL))
    {
        TokenType op = peek().type;
        advance();

        auto right = parsePrimary();

        return std::make_unique<BinaryExpression>(
            std::move(left),
            op,
            std::move(right));
    }

    return left;
}
