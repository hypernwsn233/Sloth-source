#pragma once

#include "../ast/ast.hpp"

#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>

class CodeGenerator
{
private:

    std::ofstream output;
    std::vector<std::string> dataSection;
    std::vector<std::string> textSection;
    std::unordered_map<std::string, std::string> variableTypes;

    int stringCounter = 0;

public:

    CodeGenerator(const std::string& filename);

    void generate(const Program& program);

private:

    void generateNode(const ASTnode* node);

    void generateVariable(const VariableDeclaration* node);

    void generateFunctionCall(const FunctionCall* node);

    void emitPrintExpression(const Expression* expression);

    std::string labelFor(const std::string& name) const;

    void writeFile();
};