#include <iostream>
#include <vector>
#include <string>
#include "../parser/parser.hpp"
#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"

using namespace std;

static std::string tokenName(TokenType type)
{
  switch (type)
  {
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::COLON:
    return "COLON";
  case TokenType::SEMICOLON:
    return "SEMICOLON";
  case TokenType::TYPE_INT:
    return "TYPE_INT";
  case TokenType::ASSIGN:
    return "ASSIGN";
  case TokenType::NUMBER:
    return "NUMBER";
  case TokenType::STRING:
    return "STRING";
  case TokenType::LEFT_PAREN:
    return "LEFT_PAREN";
  case TokenType::RIGHT_PAREN:
    return "RIGHT_PAREN";
  case TokenType::EOFILE:
    return "EOFILE";
  default:
    return "UNKNOWN";
  }
  return "";
}

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), position(0) {}

Token Parser::peek()
{
  return tokens[position];
}

void Parser::advance()
{
  if (position < tokens.size())
  {
    position++;
  }
}

Token Parser::consume(TokenType expectedType)
{
  Token current = peek();
  if (peek().type == expectedType)
  {
    advance();
    return current;
  }
  else
  {
    cout << "erro de sintaxe: esperando " << tokenName(expectedType) << " mas encontrado " << tokenName(peek().type) << endl;
    exit(1);
  }
}

std::unique_ptr<ASTnode> Parser::parseVariable()
{
  Token name = consume(TokenType::IDENTIFIER);
  consume(TokenType::COLON);
  Token type = consume(TokenType::TYPE_INT);
  consume(TokenType::ASSIGN);
  Token value = consume(TokenType::NUMBER);
  consume(TokenType::SEMICOLON);

  auto numberNode = std::make_unique<NumberLiteral>(std::stoi(value.value));

  return std::make_unique<VariableDeclaration>(
      name.value,
      type.value,
      std::move(numberNode));
}

std::unique_ptr<Program> Parser::parseProgram()
{
  auto program = std::make_unique<Program>();
  while (peek().type != TokenType::EOFILE) {
    
    if (peek().type == TokenType::IDENTIFIER && (position + 1) < tokens.size() && tokens[position + 1].type == TokenType::COLON) {
      auto stmt = parseVariable();
      program->addStatement(std::move(stmt));
    } else if (peek().type == TokenType::IDENTIFIER) {
      auto expr = parseFunctionCall();
      
      consume(TokenType::SEMICOLON);
      program->addStatement(std::move(expr));
    } else {
      
      advance();
    }
  }
  return program;
}

std::unique_ptr<Expression> Parser::parseExpression()
{
  Token t = peek();
  if (t.type == TokenType::NUMBER) {
    Token n = consume(TokenType::NUMBER);
    return std::make_unique<NumberLiteral>(std::stoi(n.value));
  }
  if (t.type == TokenType::STRING) {
    Token s = consume(TokenType::STRING);
    return std::make_unique<stringLiteral>(s.value);
  }
  if (t.type == TokenType::IDENTIFIER) {
    // Could be identifier or nested function call
    if ((position + 1) < tokens.size() && tokens[position + 1].type == TokenType::LEFT_PAREN) {
      return parseFunctionCall();
    }
    Token id = consume(TokenType::IDENTIFIER);
    return std::make_unique<Identifer>(id.value);
  }

  // Fallback
  return nullptr;
}

std::unique_ptr<Expression> Parser::parseFunctionCall()
{
  Token name = consume(TokenType::IDENTIFIER);
  consume(TokenType::LEFT_PAREN);

  std::vector<std::unique_ptr<Expression>> args;
  if (peek().type != TokenType::RIGHT_PAREN) {
    auto arg = parseExpression();
    if (arg) args.push_back(std::move(arg));
    // Note: lexer currently doesn't support commas; only single-arg calls expected
  }

  consume(TokenType::RIGHT_PAREN);

  return std::make_unique<FunctionCall>(name.value, std::move(args));
}
