#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include "../ast/ast.hpp"

class SemanticAnalyzer
{
private:
    std::unordered_map<std::string, std::string> symbolTable;
    int errorCount = 0;

public:
    void analyzeProgram(const Program &program)
    {
        // First pass: collect declarations
        for (const auto &stmt : program.statements)
        {
            if (auto var = dynamic_cast<VariableDeclaration *>(stmt.get()))
            {
                symbolTable[var->name] = var->type;
            }
        }

        // Second pass: validate
        for (const auto &stmt : program.statements)
        {
            if (auto var = dynamic_cast<VariableDeclaration *>(stmt.get()))
            {
                visit(var);
            }
            else if (auto call = dynamic_cast<FunctionCall *>(stmt.get()))
            {
                visit(call);
            }
        }

        if (errorCount > 0)
        {
            std::cerr << "Semantic errors found: " << errorCount << ". Aborting." << std::endl;
            std::exit(1);
        }
    }

    void visit(VariableDeclaration *node)
    {
        if (node->type == "int" && node->value)
        {
            if (!dynamic_cast<NumberLiteral *>(node->value.get()))
            {
                std::cerr << "Semantic error: variable '" << node->name << "' declared as int but initializer is not a number" << std::endl;
                ++errorCount;
            }
        }
        else if (node->type == "string" && node->value)
        {
            if (!dynamic_cast<stringLiteral *>(node->value.get()))
            {
                std::cerr << "Semantic error: variable '" << node->name << "' declared as string but initializer is not a string" << std::endl;
                ++errorCount;
            }
        }
        else if (node->type == "bool" && node->value)
        {
            if (!dynamic_cast<BooleanLiteral *>(node->value.get()))
            {
                std::cerr << "Semantic error: variable '" << node->name << "' declared as bool but initializer is not a boolean (true/false)" << std::endl;
                ++errorCount;
            }
        }
        // other type checks could go here
    }

    void visit(FunctionCall *node)
    {
        if (node->functionName == "sayf" || node->functionName == "print")
        {
            if (node->arguments.empty())
            {
                std::cerr << "Semantic error: " << node->functionName << " expects 1 argument" << std::endl;
                ++errorCount;
                return;
            }
            auto &arg = node->arguments[0];
            if (dynamic_cast<NumberLiteral *>(arg.get()))
                return;
            if (dynamic_cast<stringLiteral *>(arg.get()))
                return;
            if (dynamic_cast<BooleanLiteral *>(arg.get()))
                return;
            if (auto id = dynamic_cast<Identifer *>(arg.get()))
            {
                if (symbolTable.find(id->name) == symbolTable.end())
                {
                    std::cerr << "Semantic error: unknown identifier '" << id->name << "' in call to " << node->functionName << std::endl;
                    ++errorCount;
                }
                return;
            }
            std::cerr << "Semantic error: unsupported argument type in call to " << node->functionName << std::endl;
            ++errorCount;
        }
        else if (node->functionName == "scan")
        {
            // scan(varName)
            if (node->arguments.size() != 1)
            {
                std::cerr << "Semantic error: scan expects 1 argument" << std::endl;
                ++errorCount;
                return;
            }
            if (auto id = dynamic_cast<Identifer *>(node->arguments[0].get()))
            {
                auto it = symbolTable.find(id->name);
                if (it == symbolTable.end())
                {
                    std::cerr << "Semantic error: unknown identifier '" << id->name << "' in scan" << std::endl;
                    ++errorCount;
                }
                else if (it->second != "int")
                {
                    std::cerr << "Semantic error: scan target '" << id->name << "' must be int" << std::endl;
                    ++errorCount;
                }
            }
            else
            {
                std::cerr << "Semantic error: scan argument must be an identifier" << std::endl;
                ++errorCount;
            }
        }
    }
};
