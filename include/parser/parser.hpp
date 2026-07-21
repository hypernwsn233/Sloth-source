#pragma once

#include <vector>
#include <memory>

#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"

// ============================================================================
//  Parser (analisador sintático)
//
//  Recebe a lista de tokens do lexer e monta a AST (árvore sintática abstrata)
//  — a representação estruturada do programa. É um parser recursivo-descendente:
//  cada regra da gramática vira um método.
//
//  Gramática suportada:
//      programa       ::= (declaracao | chamada)*
//      declaracao     ::= IDENTIFIER ':' TYPE '=' expressao ';'
//      chamada        ::= IDENTIFIER '(' argumentos? ')' ';'
//      argumentos     ::= expressao (',' expressao)*
//      expressao      ::= NUMBER | STRING | IDENTIFIER | chamada
// ============================================================================

class Parser
{
public:
    explicit Parser(std::vector<Token> tokens);

    // Ponto de entrada: consome todos os tokens e devolve o programa completo.
    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> tokens;
    size_t position = 0;

    // --- Navegação pelos tokens ---
    const Token &peek() const;                // token atual
    const Token &peekAt(size_t offset) const; // token à frente
    bool check(TokenType type) const;         // o token atual é deste tipo?
    bool atEnd() const;
    void advance();
    Token consume(TokenType expected); // exige um tipo; erro se não bater

    // --- Regras da gramática ---
    std::unique_ptr<ASTnode> parseStatement();
    std::unique_ptr<ASTnode> parseVariable();
    std::unique_ptr<Expression> parseFunctionCall();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parsePrimary();

    // Reporta um erro de sintaxe com linha/coluna e encerra o programa.
    [[noreturn]] void error(const std::string &message) const;
};
