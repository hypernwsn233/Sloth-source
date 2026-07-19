#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>

#include "include/lexer/lexer.hpp"
#include "include/parser/parser.hpp"
#include "include/semantica/seman.hpp"
#include "include/compiler/CodeGenerator.hpp"


// ---------------------------------------------------------------------------
//  Opções de linha de comando.
// ---------------------------------------------------------------------------
struct Options
{
    std::string inputPath = "test/index.sth";
    std::string outputPath;                 // vazio -> derivado do alvo
    Target      target = Target::host();
    int         optLevel = 1;
    bool        showAsm = false;            // -asm
    bool        verbose = false;            // -v : imprime tokens/AST
    bool        assembleExec = true;        // gera executável (desliga com -S)
    bool        showHelp = false;
};

static void printUsage(const char* prog)
{
    std::cout <<
        "Uso: " << prog << " [opcoes] [arquivo.sth]\n\n"
        "Opcoes:\n"
        "  -asm                Mostra o codigo assembly gerado no terminal\n"
        "  -S                  Gera apenas o .s (nao monta o executavel)\n"
        "  -o <arquivo>        Nome do arquivo de saida\n"
        "  -target <so>        Alvo: linux | macos | windows (padrao: host)\n"
        "  -O0 | -O1 | -O2     Nivel de otimizacao peephole (padrao: -O1)\n"
        "  -v                  Modo verboso (tokens + AST)\n"
        "  -h, --help          Mostra esta ajuda\n";
}

static bool parseArgs(int argc, char** argv, Options& opt)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];

        if (a == "-asm")            opt.showAsm = true;
        else if (a == "-S")         opt.assembleExec = false;
        else if (a == "-v")         opt.verbose = true;
        else if (a == "-h" || a == "--help") { opt.showHelp = true; }
        else if (a == "-O0")        opt.optLevel = 0;
        else if (a == "-O1")        opt.optLevel = 1;
        else if (a == "-O2")        opt.optLevel = 2;
        else if (a == "-o")
        {
            if (i + 1 >= argc) { std::cerr << "-o exige um argumento\n"; return false; }
            opt.outputPath = argv[++i];
        }
        else if (a == "-target")
        {
            if (i + 1 >= argc) { std::cerr << "-target exige um argumento\n"; return false; }
            try { opt.target = Target::fromString(argv[++i]); }
            catch (const std::exception& e) { std::cerr << e.what() << "\n"; return false; }
        }
        else if (!a.empty() && a[0] == '-')
        {
            std::cerr << "Opcao desconhecida: " << a << "\n";
            return false;
        }
        else
        {
            opt.inputPath = a; // argumento posicional = arquivo de entrada
        }
    }
    return true;
}


int main(int argc, char** argv)
{
    Options opt;
    if (!parseArgs(argc, argv, opt)) { printUsage(argv[0]); return 1; }
    if (opt.showHelp) { printUsage(argv[0]); return 0; }

    std::ifstream file(opt.inputPath);
    if (!file)
    {
        std::cerr << "Erro ao abrir " << opt.inputPath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    if (opt.verbose)
    {
        std::cout << "=== CODIGO FONTE ===\n" << source << "\n";
    }

    // Lexer.
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (opt.verbose)
    {
        std::cout << "\n=== TOKENS (LEXER) ===\n";
        for (const auto &token : tokens)
            std::cout << tokenTypeName(token.type) << " -> '" << token.value << "'\n";
    }

    // Parser.
    Parser parser(tokens);
    auto program = parser.parseProgram();

    // Analise semantica.
    SemanticAnalyzer semanticAnalyzer;
    semanticAnalyzer.analyzeProgram(*program);

    if (opt.verbose)
    {
        std::cout << "\n=== AST ===\n";
        program->sayf(0);
    }

    // Nome do arquivo .s temporario e do executavel de saida.
    const std::string asmPath = "out.s";
    std::string execPath = opt.outputPath.empty()
                         ? ("out_exec" + opt.target.execExtension())
                         : opt.outputPath;

    try
    {
        CodeGenerator cg(asmPath, opt.target, opt.optLevel);
        cg.generate(*program);

        // Flag -asm: exibe o assembly gerado no terminal.
        if (opt.showAsm)
        {
            std::cout << "\n=== ASSEMBLY GERADO (" << opt.target.name() << ") ===\n";
            std::cout << cg.assemblyText();

            const auto& s = cg.optimizationStats();
            std::cout << "\n--- Otimizacao (nivel " << opt.optLevel << ") ---\n";
            std::cout << "  movs redundantes removidos : " << s.redundantMovsRemoved << "\n";
            std::cout << "  pares push/pop removidos    : " << s.pushPopPairsRemoved << "\n";
            std::cout << "  aritmetica no-op removida   : " << s.noopArithRemoved << "\n";
            std::cout << "  idiomas de zero (xor)       : " << s.zeroIdiomsApplied << "\n";
            std::cout << "  labels mortos removidos     : " << s.deadLabelsRemoved << "\n";
            std::cout << "  passagens executadas        : " << s.totalPasses << "\n\n";
        }

        std::cout << "Arquivo assembly gerado: " << asmPath << "\n";

        // Montagem/ligacao: so faz sentido montar nativamente quando o alvo
        // coincide com o host. Cross-compilar exige um toolchain especifico.
        if (opt.assembleExec)
        {
            bool sameAsHost =
                (opt.target.os() == Target::host().os());

            if (!sameAsHost)
            {
                std::cout << "Aviso: alvo (" << opt.target.name()
                          << ") difere do host; apenas o .s foi gerado.\n"
                          << "Monte-o em um toolchain do alvo para obter o executavel.\n";
            }
            else
            {
                std::string cmd = "gcc " + asmPath + " -no-pie -o " + execPath;
                int rc = std::system(cmd.c_str());
                if (rc != 0)
                {
                    std::cerr << "Erro ao montar/ligar " << asmPath << "\n";
                    return 1;
                }
                std::remove(asmPath.c_str());
                std::cout << "Executavel gerado: " << execPath << "\n";
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro no CodeGenerator: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nPipeline concluido com sucesso.\n";
    return 0;
}
