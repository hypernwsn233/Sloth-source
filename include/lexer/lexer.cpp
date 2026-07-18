#include "./lexer.hpp"
#include <cctype>

Lexer::Lexer(const std::string& source){
  this->source = source;
  this->position = 0;
};

std::vector<Token> Lexer::tokenize(){
  std::vector<Token> tokens;
  while(position < source.length()){
    char currentChar = source[position];
    
    if(isspace(currentChar)){
      position++;
      continue;
    }

    if(isalpha(currentChar)){
      std::string word;
      while(position < source.length() && isalnum(source[position])){
        word += source[position];
        position++;
      }
      if(word == "int"){
        tokens.push_back({TokenType::TYPE_INT, word});
      } else {
        tokens.push_back({TokenType::IDENTIFIER, word});
      }
      continue;
    }

    if(isdigit(currentChar)){
      std::string number;
      while(position < source.length() && isdigit(source[position])){
        number += source[position];
        position++;
      }
      tokens.push_back({TokenType::NUMBER, number});
      continue;
    }
    
    if(currentChar == '"'){
      std::string stringLiteral;
      position++;
      while(position < source.length() && source[position] != '"'){
        stringLiteral += source[position];
        position++;
      }
      position++;
      tokens.push_back({TokenType::STRING, stringLiteral});
      continue;
    }

    switch(currentChar){
      case ':':
        tokens.push_back({TokenType::COLON, ":"});
        position++;
        break;
      case ';':
        tokens.push_back({TokenType::SEMICOLON, ";"});
        position++;
        break;
      case '=':
        tokens.push_back({TokenType::ASSIGN, "="});
        position++;
        break;
      case '(':
        tokens.push_back({TokenType::LEFT_PAREN, "("});
        position++;
        break;
      case ')':
        tokens.push_back({TokenType::RIGHT_PAREN, ")"});
        position++;
        break;
      default:
        position++;
        break;
    }
    
  }
  tokens.push_back({TokenType::EOFILE, ""});
  return tokens;
}