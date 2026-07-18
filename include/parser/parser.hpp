#pragma once

#include <vector>
#include <memory>

#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"


class Parser {
private:
    std::vector<Token> tokens;
    int position;

public:
    Parser(std::vector<Token> tokens);

    std::unique_ptr<ASTnode> parseVariable();
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseFunctionCall();

private:
    Token peek();
    void advance();
    Token consume(TokenType expectedType);
};