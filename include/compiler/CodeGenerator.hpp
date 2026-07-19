#pragma once

#include "../ast/ast.hpp"
#include "../otimizer/Target.hpp"
#include "../otimizer/AssemblyOptimizer.hpp"

#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>

class CodeGenerator
{
private:

    std::ofstream output;
    std::string   outputPath;
    std::vector<std::string> dataSection;
    std::vector<std::string> textSection;
    std::unordered_map<std::string, std::string> variableTypes;

    Target target;
    int     optLevel;

    int stringCounter = 0;

    OptimizationStats lastStats;

public:

    // filename  : arquivo .s de saída
    // target    : arquitetura + SO alvo (padrão = host)
    // optLevel  : nível de otimização peephole (0 = off, 1 = seguro, 2 = agressivo)
    CodeGenerator(const std::string& filename,
                  Target target = Target::host(),
                  int optLevel = 1);

    void generate(const Program& program);

    // Retorna todo o assembly final (já otimizado) como uma única string.
    // Usado pela flag -asm para exibir o código no terminal.
    std::string assemblyText() const;

    const OptimizationStats& optimizationStats() const { return lastStats; }

    const Target& getTarget() const { return target; }

private:

    void generateNode(const ASTnode* node);

    void generateVariable(const VariableDeclaration* node);

    void generateFunctionCall(const FunctionCall* node);

    void emitPrintExpression(const Expression* expression);

    // Garante que as strings "true"/"false" existam na secao de dados
    // (usadas para imprimir valores booleanos).
    void ensureBoolStrings();

    std::string labelFor(const std::string& name) const;

    // Monta as linhas finais (data + extern + text) em `out`.
    void assemble(std::vector<std::string>& out) const;

    void writeFile();
};
