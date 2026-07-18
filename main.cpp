#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "include/lexer/lexer.hpp"
#include "include/parser/parser.hpp"
#include "include/semantica/seman.hpp"
#include <cstdlib>
#include <cstdio>
#include "include/compiler/CodeGenerator.hpp"

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
}

int main()
{
    std::ifstream file("test/index.sth");
    if (!file)
    {
        std::cerr << "Erro ao abrir test/index.sth" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    std::cout << "=== CODIGO FONTE ===" << std::endl;
    std::cout << source << std::endl;

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\n=== TOKENS (LEXER) ===" << std::endl;
    for (const auto &token : tokens)
    {
        std::cout << tokenName(token.type) << " -> '" << token.value << "'" << std::endl;
    }

    Parser parser(tokens);
    auto program = parser.parseProgram();
    
    SemanticAnalyzer semanticAnalyzer;

    semanticAnalyzer.analyzeProgram(*program);

    std::cout << "\n=== AST ===" << std::endl;
    program->sayf(0);

    // Generate assembly
    try
    {
        CodeGenerator cg("out.s");
        cg.generate(*program);
        std::cout << "Arquivo de assembly gerado: out.s" << std::endl;

        // Assemble and link into executable, then remove assembly file
        std::string cmd = "gcc out.s -no-pie -o out_exec";
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            std::cerr << "Erro ao montar/ligar out.s" << std::endl;
            return 1;
        }
        if (std::remove("out.s") != 0) {
            std::cerr << "Aviso: nao foi possivel apagar out.s" << std::endl;
        }
        std::cout << "Executavel gerado: out_exec" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro no CodeGenerator: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nPipeline concluido com sucesso." << std::endl;

    return 0;
}