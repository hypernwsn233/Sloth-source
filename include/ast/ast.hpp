#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../lexer/lexer.hpp"

// Classe pra todos os NOS do A/DST(DOENÇAS CATASTROFICAS)

class ASTnode
{
public:
  virtual ~ASTnode() = default;

  virtual void sayf(int ident) const = 0;
};

// identação

inline void printIndent(int ident)
{
  for (int i = 0; i < ident; i++)
  {
    std::cout << "  ";
  }
}

// EXPRESSIONS

class Expression : public ASTnode
{
public:
  virtual ~Expression() = default;

  virtual std::string getType() const = 0;
};

class NumberLiteral : public Expression
{
public:
  int value;

  NumberLiteral(int value)
      : value(value)
  {
  }

  std::string getType() const override
  {
    return "number";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "NumberLiteral: "
              << value
              << std::endl;
  }
};

class BooleanLiteral : public Expression
{
public:
  bool value;

  BooleanLiteral(bool value)
      : value(value)
  {
  }

  std::string getType() const override
  {
    return "bool";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "BooleanLiteral: "
              << (value ? "true" : "false")
              << std::endl;
  }
};

class stringLiteral : public Expression
{
public:
  std::string value;

  stringLiteral(std::string value)
      : value(value)
  {
  }

  std::string getType() const override
  {
    return "string";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "StringLiteral: "
              << value
              << std::endl;
  }
};

class Identifer : public Expression
{
public:
  std::string name;

  Identifer(std::string name)
      : name(name)
  {
  }

  std::string getType() const override
  {
    return "identifier";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "Identifier: "
              << name
              << std::endl;
  }
};

// DECLARATIONS
class VariableDeclaration : public ASTnode
{
public:
  std::string name;
  std::string type;
  std::unique_ptr<Expression> value;

  VariableDeclaration(
      const std::string &name, const std::string &type, std::unique_ptr<Expression> value)
      : name(name), type(type), value(std::move(value)) {}

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "VariableDeclaration: " << name << " of type " << type << std::endl;
    if (value)
    {
      value->sayf(ident + 1);
    }
  }
};

// chamada de funçãp
class FunctionCall : public Expression
{
public:
  std::string functionName;
  std::vector<std::unique_ptr<Expression>> arguments;

  FunctionCall(
      const std::string &functionName,
      std::vector<std::unique_ptr<Expression>> arguments)
      : functionName(functionName),
        arguments(std::move(arguments))
  {
  }

  std::string getType() const override
  {
    return "function_call";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);

    std::cout
        << "FunctionCall: "
        << functionName
        << std::endl;

    for (const auto &arg : arguments)
    {
      arg->sayf(ident + 1);
    }
  }
};

class Program : public ASTnode
{
public:
  std::vector<std::unique_ptr<ASTnode>> statements;

  void addStatement(std::unique_ptr<ASTnode> stmt)
  {
    statements.push_back(std::move(stmt));
  }

  void sayf(int indent = 0) const override
  {
    printIndent(indent);
    std::cout << "Program\n";

    for (const auto &stmt : statements)
      stmt->sayf(indent + 1);
  }
};

class BinaryExpression : public Expression
{
public:
  std::unique_ptr<Expression> left;
  TokenType op;
  std::unique_ptr<Expression> right;

  BinaryExpression(
      std::unique_ptr<Expression> left,
      TokenType op,
      std::unique_ptr<Expression> right)
      : left(std::move(left)),
        op(op),
        right(std::move(right))
  {
  }

  std::string getType() const override
  {
    return "bool";
  }

  void sayf(int ident) const override
  {
    printIndent(ident);
    std::cout << "BinaryExpression\n";

    left->sayf(ident + 1);

    printIndent(ident + 1);
    std::cout << tokenTypeName(op) << "\n";

    right->sayf(ident + 1);
  }
};
