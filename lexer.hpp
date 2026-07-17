#pragma once
#include <string>
#include <vector>

enum class TokenType{
  IDENTIFIER,
  COLON,
  SEMICOLON,
  TYPE_INT,
  ASSIGN,
  NUMBER,
  STRING,
  LEFT_PAREN,
  RIGHT_PAREN,
  EOFILE
};

struct Token{
  TokenType type;
  std::string value;
};

class Lexer {
  private:
  std::string source;
  int position;
  public:
  Lexer(const std::string& source);
  std::vector<Token> tokenize();
};
