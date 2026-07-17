#include <iostream>
#include "lexer.hpp"
#include <fstream>
using namespace std;
std::string tokenName(TokenType type) {
    switch(type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::TYPE_INT: return "TYPE_INT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::EOFILE: return "EOFILE";
        default: return "UNKNOWN";
    }
    return "";
}

int main(){
  fstream file("./test/index.sth", ios::in);
  if(!file.is_open()){
    cerr << "Error opening file" << endl;
    return 1;
  }
  string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
  Lexer lexer(source);
  auto tokens = lexer.tokenize();
  for(const auto& token : tokens){
    cout << tokenName(token.type) << ": " << token.value << endl;
  }
  file.close();
  return 0;
}