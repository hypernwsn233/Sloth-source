#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>

#include "include/lexer/lexer.hpp"
#include "include/parser/parser.hpp"
#include "include/semantica/seman.hpp"
#include "include/compiler/CodeGenerator.hpp"


static std::string tokenName(TokenType type)
{
    switch (type)
    {
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
}


int main()
{
    std::ifstream file("test/index.sth");

    if (!file)
    {
        std::cerr << "Erron ao abrir test/index.sth" << std::endl;
        return 1;
    }


    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();


    std::cout << "=== CODIGO FONTE ===\n";
    std::cout << source << "\n";


    Lexer lexer(source);

    std::vector<Token> tokens = lexer.tokenize();


    std::cout << "\n=== TOKENS (LEXER) ===\n";

    for (const auto &token : tokens)
    {
        std::cout 
            << tokenName(token.type)
            << " -> '"
            << token.value
            << "'\n";
    }


    Parser parser(tokens);

    auto program = parser.parseProgram();


    SemanticAnalyzer semanticAnalyzer;

    semanticAnalyzer.analyzeProgram(*program);



    std::cout << "\n=== AST ===\n";

    program->sayf(0);



    try
    {
        CodeGenerator cg("out.s");

        cg.generate(*program);


        std::cout << "Arquivo assembly gerado: out.s\n";


        std::string cmd = "gcc out.s -no-pie -o out_exec";


        int rc = std::system(cmd.c_str());


        if (rc != 0)
        {
            std::cerr << "Erro ao montar/ligar out.s\n";
            return 1;
        }


        std::remove("out.s");


        std::cout << "Executavel gerado: out_exec\n";
    }
    catch (const std::exception &e)
    {
        std::cerr 
            << "Erro no CodeGenerator: "
            << e.what()
            << std::endl;

        return 1;
    }
    
    std::cout << "\nPipeline concluido com sucesso.\n";

    return 0;
}