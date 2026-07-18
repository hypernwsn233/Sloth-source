#pragma once

#include "../ast/ast.hpp"

#include <fstream>
#include <string>
#include <vector>

class CodeGenerator
{
private:

    std::ofstream output;

    std::vector<std::string> dataSection;
    std::vector<std::string> textSection;

    int stringCounter = 0;

public:

    CodeGenerator(const std::string& filename);

    void generate(const Program& program);

private:

    void generateNode(const ASTnode* node);

    void generateVariable(const VariableDeclaration* node);

    void generateFunctionCall(const FunctionCall* node);

    void writeFile();
};