#pragma once

#include <string>
#include <vector>

// ============================================================================
//  AssemblyOptimizer.hpp
//
//  Otimizador "peephole" (janela deslizante) que roda sobre a lista de
//  instruções de assembly já geradas pelo CodeGenerator, antes de escrevê-las
//  no arquivo. Opera puramente em texto GAS/AT&T x86-64 e aplica um conjunto
//  de transformações seguras e clássicas de compiladores:
//
//    1. Remoção de movs redundantes    (mov %rax, %rax)
//    2. Coalescência de mov encadeado  (mov A,B ; mov B,A  ->  mov A,B)
//    3. Eliminação de push/pop mortos   em pares imediatos do mesmo registrador
//    4. Simplificação de add/sub $0     (no-op aritmético)
//    5. Uso de xor reg,reg no lugar de mov $0,reg  (menor e quebra dependência)
//    6. Remoção de labels não referenciados (dead label elimination)
//    7. Compactação de linhas em branco redundantes
//
//  As passagens são idempotentes e rodam em ponto-fixo (repetem até estabilizar)
//  para capturar oportunidades expostas por passagens anteriores.
// ============================================================================

struct OptimizationStats
{
    int redundantMovsRemoved = 0;
    int pushPopPairsRemoved  = 0;
    int noopArithRemoved     = 0;
    int zeroIdiomsApplied    = 0;
    int deadLabelsRemoved    = 0;
    int totalPasses          = 0;

    int totalRemoved() const
    {
        return redundantMovsRemoved + pushPopPairsRemoved +
               noopArithRemoved + deadLabelsRemoved;
    }
};

class AssemblyOptimizer
{
public:
    // level 0 = desliga; 1 = seguro (padrão); 2 = agressivo (idiomas extras).
    explicit AssemblyOptimizer(int level = 1) : level_(level) {}

    // Otimiza a seção .text in-place. Retorna estatísticas do que foi feito.
    OptimizationStats optimize(std::vector<std::string>& text);

    const OptimizationStats& stats() const { return stats_; }

private:
    int level_;
    OptimizationStats stats_;

    // Passagens individuais; cada uma retorna true se alterou algo.
    bool passRedundantMov(std::vector<std::string>& t);
    bool passPushPop(std::vector<std::string>& t);
    bool passNoopArith(std::vector<std::string>& t);
    bool passZeroIdiom(std::vector<std::string>& t);
    bool passDeadLabels(std::vector<std::string>& t);
};
