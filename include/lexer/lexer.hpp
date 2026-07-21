#pragma once

#include <string>
#include <vector>

// ============================================================================
//  Lexer (analisador léxico)
//
//  Transforma o texto-fonte em uma sequência de "tokens" — as menores unidades
//  com significado da linguagem (identificadores, números, símbolos, etc.).
//
//  Exemplo:
//      value: string = "a";
//  vira:
//      IDENTIFIER(value) COLON TYPE_VALUE(string) ASSIGN STRING(a) SEMICOLON
// ============================================================================

enum class TokenType
{
    IDENTIFIER,  // nomes: value, sayf, x ...
    COLON,       // :
    SEMICOLON,   // ;
    COMMA,       // ,
    TYPE_VALUE,  // palavras-chave de tipo: int, string, boolean, auto
    ASSIGN,      // =
    NUMBER,      // 42
    STRING,      // "texto"
    BOOLEAN,     // true/false
    LEFT_PAREN,  // (
    RIGHT_PAREN, // )
    EOFILE,      // fim do arquivo
    UNKNOWN,     // caractere não reconhecido
    GREATER,     // calma ae vou pesquisar
    LESS,        // nao lembro
    EQUAL_EQUAL, // engual
    NOT_EQUAL    // nao engual
};

struct Token
{
    TokenType type;
    std::string value;
    int line = 0;   // linha onde o token começa (1-based)
    int column = 0; // coluna onde o token começa (1-based)
};

// Nome legível de um tipo de token — usado em mensagens de erro e depuração.
// Centralizado aqui para não ficar duplicado no parser e no main.
std::string tokenTypeName(TokenType type);

class Lexer
{
public:
    explicit Lexer(const std::string &source);

    // Percorre todo o texto-fonte e devolve a lista de tokens,
    // sempre terminada por um token EOFILE.
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t position = 0; // índice do próximo caractere a ler
    int line = 1;
    int column = 1;

    // --- Navegação pelo texto ---
    bool atEnd() const;    // já chegamos ao fim?
    char current() const;  // caractere atual (sem consumir)
    char peekNext() const; // próximo caractere (sem consumir)
    void advance();        // avança uma posição, atualizando linha/coluna

    // --- Reconhecimento de cada tipo de token ---
    Token readWord();   // identificador, palavra-chave de tipo ou true/false
    Token readNumber(); // sequência de dígitos
    Token readString(); // "texto entre aspas"
};
