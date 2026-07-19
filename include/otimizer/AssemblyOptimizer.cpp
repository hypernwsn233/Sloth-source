#include "AssemblyOptimizer.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

// ============================================================================
//  Helpers de parsing textual das instruções.
//  Trabalhamos com o formato GAS/AT&T já produzido pelo CodeGenerator, no qual
//  cada elemento de `text` é uma linha (com indentação de 4 espaços para
//  instruções e sem indentação para labels/diretivas).
// ============================================================================

namespace
{
    // Remove espaços das pontas.
    std::string trim(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }

    bool isBlank(const std::string& s) { return trim(s).empty(); }

    // É uma diretiva do montador? (.globl, .type, .section, ...)
    bool isDirective(const std::string& s)
    {
        std::string t = trim(s);
        return !t.empty() && t[0] == '.';
    }

    // É um label? ("main:", ".LC1:")  -> termina em ':' sem espaços internos.
    bool isLabel(const std::string& s)
    {
        std::string t = trim(s);
        if (t.size() < 2 || t.back() != ':') return false;
        // não pode ter espaço antes dos dois-pontos (senão é instrução)
        return t.find(' ') == std::string::npos;
    }

    // Mnemônico da instrução (primeira palavra), vazio se não for instrução.
    std::string mnemonic(const std::string& s)
    {
        std::string t = trim(s);
        if (t.empty() || isDirective(t) || isLabel(t)) return "";
        size_t sp = t.find_first_of(" \t");
        return sp == std::string::npos ? t : t.substr(0, sp);
    }

    // Operandos (tudo após o mnemônico), sem trim individual dos itens.
    std::string operands(const std::string& s)
    {
        std::string t = trim(s);
        size_t sp = t.find_first_of(" \t");
        return sp == std::string::npos ? "" : trim(t.substr(sp + 1));
    }

    // Divide "a, b" em {"a","b"} respeitando o formato simples do gerador.
    std::vector<std::string> splitOperands(const std::string& ops)
    {
        std::vector<std::string> out;
        std::string cur;
        int paren = 0;
        for (char c : ops)
        {
            if (c == '(') paren++;
            if (c == ')') paren--;
            if (c == ',' && paren == 0)
            {
                out.push_back(trim(cur));
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        if (!trim(cur).empty()) out.push_back(trim(cur));
        return out;
    }

    // Reconstrói uma linha de instrução com a indentação padrão do gerador.
    std::string emit(const std::string& mnem, const std::string& ops)
    {
        return "    " + mnem + " " + ops;
    }
}

// ---------------------------------------------------------------------------
//  Passagem 1: remove movs cujo destino == origem (mov %rax,%rax etc).
// ---------------------------------------------------------------------------
bool AssemblyOptimizer::passRedundantMov(std::vector<std::string>& t)
{
    bool changed = false;
    std::vector<std::string> out;
    out.reserve(t.size());

    for (const auto& line : t)
    {
        std::string m = mnemonic(line);
        if (m == "movq" || m == "movl" || m == "mov")
        {
            auto ops = splitOperands(operands(line));
            if (ops.size() == 2 && ops[0] == ops[1])
            {
                stats_.redundantMovsRemoved++;
                changed = true;
                continue; // descarta
            }
        }
        out.push_back(line);
    }

    t.swap(out);
    return changed;
}

// ---------------------------------------------------------------------------
//  Passagem 2: elimina pares push/pop imediatos do MESMO registrador quando
//  nada entre eles usa a pilha — padrão "pushq %rX \n popq %rX".
// ---------------------------------------------------------------------------
bool AssemblyOptimizer::passPushPop(std::vector<std::string>& t)
{
    bool changed = false;
    std::vector<std::string> out;
    out.reserve(t.size());

    for (size_t i = 0; i < t.size(); ++i)
    {
        if (i + 1 < t.size())
        {
            std::string m1 = mnemonic(t[i]);
            std::string m2 = mnemonic(t[i + 1]);
            if ((m1 == "pushq" || m1 == "push") &&
                (m2 == "popq"  || m2 == "pop"))
            {
                if (operands(t[i]) == operands(t[i + 1]))
                {
                    stats_.pushPopPairsRemoved++;
                    changed = true;
                    i++;      // pula ambos
                    continue;
                }
            }
        }
        out.push_back(t[i]);
    }

    t.swap(out);
    return changed;
}

// ---------------------------------------------------------------------------
//  Passagem 3: remove aritmética no-op — add/sub com imediato $0.
// ---------------------------------------------------------------------------
bool AssemblyOptimizer::passNoopArith(std::vector<std::string>& t)
{
    bool changed = false;
    std::vector<std::string> out;
    out.reserve(t.size());

    for (const auto& line : t)
    {
        std::string m = mnemonic(line);
        if (m == "addq" || m == "addl" || m == "subq" || m == "subl" ||
            m == "add"  || m == "sub")
        {
            auto ops = splitOperands(operands(line));
            if (ops.size() == 2 && (ops[0] == "$0"))
            {
                stats_.noopArithRemoved++;
                changed = true;
                continue;
            }
        }
        out.push_back(line);
    }

    t.swap(out);
    return changed;
}

// ---------------------------------------------------------------------------
//  Passagem 4 (nível >= 2): troca "movl $0, %reg" por "xorl %reg,%reg".
//  Instrução menor (2 bytes) e quebra a cadeia de dependência do registrador.
//  Só aplicável a registradores (não a memória) e a imediato zero.
// ---------------------------------------------------------------------------
bool AssemblyOptimizer::passZeroIdiom(std::vector<std::string>& t)
{
    if (level_ < 2) return false;

    bool changed = false;
    for (auto& line : t)
    {
        std::string m = mnemonic(line);
        if (m != "movl" && m != "movq") continue;

        auto ops = splitOperands(operands(line));
        if (ops.size() != 2) continue;
        if (ops[0] != "$0") continue;
        if (ops[1].empty() || ops[1][0] != '%') continue; // deve ser registrador

        // xorl sempre limpa os 64 bits superiores; usamos a forma de 32 bits.
        std::string reg = ops[1];
        // normaliza r?x de 64 -> 32 quando veio de movq (opcional, mantemos simples)
        std::string newLine = emit("xorl", reg + ", " + reg);
        if (newLine != line)
        {
            line = newLine;
            stats_.zeroIdiomsApplied++;
            changed = true;
        }
    }
    return changed;
}

// ---------------------------------------------------------------------------
//  Passagem 5: elimina labels locais (.L...) que não são referenciados por
//  nenhuma instrução. Labels globais (main, etc.) nunca são removidos.
// ---------------------------------------------------------------------------
bool AssemblyOptimizer::passDeadLabels(std::vector<std::string>& t)
{
    bool changed = false;

    // Coleta labels definidos e verifica referências textuais.
    std::vector<std::string> out;
    out.reserve(t.size());

    for (const auto& line : t)
    {
        if (isLabel(line))
        {
            std::string lbl = trim(line);
            lbl.pop_back(); // remove ':'

            // Só consideramos labels locais (começam com ".L") como candidatos.
            if (lbl.rfind(".L", 0) == 0)
            {
                bool referenced = false;
                for (const auto& other : t)
                {
                    if (&other == &line) continue;
                    if (isLabel(other)) continue;
                    // referência aparece como "lbl(" ou "lbl " ou fim de linha
                    if (other.find(lbl) != std::string::npos)
                    {
                        referenced = true;
                        break;
                    }
                }
                if (!referenced)
                {
                    stats_.deadLabelsRemoved++;
                    changed = true;
                    continue; // descarta o label morto
                }
            }
        }
        out.push_back(line);
    }

    t.swap(out);
    return changed;
}

// ---------------------------------------------------------------------------
//  Orquestra as passagens em ponto-fixo.
// ---------------------------------------------------------------------------
OptimizationStats AssemblyOptimizer::optimize(std::vector<std::string>& text)
{
    stats_ = OptimizationStats{};
    if (level_ <= 0) return stats_;

    bool changed = true;
    const int kMaxPasses = 16; // trava de segurança contra oscilação
    while (changed && stats_.totalPasses < kMaxPasses)
    {
        changed = false;
        changed |= passRedundantMov(text);
        changed |= passPushPop(text);
        changed |= passNoopArith(text);
        changed |= passZeroIdiom(text);
        changed |= passDeadLabels(text);
        stats_.totalPasses++;
    }

    return stats_;
}