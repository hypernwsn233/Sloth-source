#pragma once

#include <string>
#include <array>
#include <stdexcept>

// ============================================================================
//  Target.hpp
//
//  Abstração de "alvo" (arquitetura + sistema operacional) para a geração de
//  código. O mesmo AST é traduzido para assembly x86-64, mas cada SO tem:
//
//    * Convenção de chamada diferente
//        - System V AMD64 (Linux / macOS): rdi, rsi, rdx, rcx, r8, r9
//        - Microsoft x64  (Windows):       rcx, rdx, r8,  r9  (+ 32B shadow)
//    * Decoração de símbolos diferente
//        - macOS Mach-O prefixa "_" em símbolos externos e no entrypoint
//    * Diretivas de seção diferentes
//        - Linux ELF usa .note.GNU-stack; Mach-O/PE não.
//
//  Toda essa variação fica encapsulada aqui para o CodeGenerator permanecer
//  limpo e genérico.
// ============================================================================

enum class TargetOS
{
    Linux,
    MacOS,
    Windows
};

class Target
{
public:
    explicit Target(TargetOS os) : os_(os) {}

    TargetOS os() const { return os_; }

    // Nome legível do alvo (usado em logs / banner do -asm).
    std::string name() const
    {
        switch (os_)
        {
            case TargetOS::Linux:   return "x86-64 Linux (ELF, System V AMD64)";
            case TargetOS::MacOS:   return "x86-64 macOS (Mach-O, System V AMD64)";
            case TargetOS::Windows: return "x86-64 Windows (PE, Microsoft x64)";
        }
        return "desconhecido";
    }

    // Sufixo do arquivo objeto/executável padrão.
    std::string execExtension() const
    {
        return os_ == TargetOS::Windows ? ".exe" : "";
    }

    // ---- Decoração de símbolos --------------------------------------------
    // Mach-O prefixa "_" em símbolos globais/externos. ELF e PE não.
    std::string mangle(const std::string& sym) const
    {
        return os_ == TargetOS::MacOS ? "_" + sym : sym;
    }

    // Entrypoint do programa.
    std::string entry() const { return mangle("main"); }

    // ---- Convenção de chamada ---------------------------------------------
    // Registrador inteiro/ponteiro de 64 bits para o argumento `index` (0-based).
    // Lança se o índice exceder os registradores disponíveis (spill não suportado).
    std::string argReg64(int index) const
    {
        static const std::array<const char*, 6> sysv =
            {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        static const std::array<const char*, 4> win =
            {"rcx", "rdx", "r8", "r9"};

        if (isSysV())
        {
            if (index < 0 || index >= (int)sysv.size())
                throw std::out_of_range("argumento fora dos registradores System V");
            return std::string("%") + sysv[index];
        }
        if (index < 0 || index >= (int)win.size())
            throw std::out_of_range("argumento fora dos registradores Microsoft x64");
        return std::string("%") + win[index];
    }

    // Versão de 32 bits do registrador de argumento (para ints).
    std::string argReg32(int index) const
    {
        static const std::array<const char*, 6> sysv =
            {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
        static const std::array<const char*, 4> win =
            {"ecx", "edx", "r8d", "r9d"};

        if (isSysV())
        {
            if (index < 0 || index >= (int)sysv.size())
                throw std::out_of_range("argumento fora dos registradores System V");
            return std::string("%") + sysv[index];
        }
        if (index < 0 || index >= (int)win.size())
            throw std::out_of_range("argumento fora dos registradores Microsoft x64");
        return std::string("%") + win[index];
    }

    bool isSysV() const
    {
        return os_ == TargetOS::Linux || os_ == TargetOS::MacOS;
    }

    // Chamada a função variádica (ex.: printf) exige, no System V, que %al
    // contenha o nº de registradores vetoriais usados (0 aqui). No Win64 não.
    bool variadicNeedsAL() const { return isSysV(); }

    // Espaço de sombra ("shadow space") de 32 bytes que o chamador Win64 deve
    // reservar na pilha antes de qualquer call.
    int shadowSpace() const { return os_ == TargetOS::Windows ? 32 : 0; }

    // ---- Seções ------------------------------------------------------------
    std::string dataSectionDirective() const
    {
        // .data funciona nos três montadores GAS-compatíveis.
        return "    .data";
    }

    std::string textSectionDirective() const
    {
        return "    .text";
    }

    // Nota de pilha não-executável: só existe/necessária em ELF (Linux).
    bool emitGNUStackNote() const { return os_ == TargetOS::Linux; }

    // Resolve o alvo a partir de uma string de linha de comando.
    static Target fromString(const std::string& s)
    {
        if (s == "linux")                       return Target(TargetOS::Linux);
        if (s == "macos" || s == "mac" || s == "darwin")
                                                return Target(TargetOS::MacOS);
        if (s == "windows" || s == "win")       return Target(TargetOS::Windows);
        throw std::invalid_argument("alvo desconhecido: " + s +
                                    " (use linux|macos|windows)");
    }

    // Alvo padrão = o SO onde o compilador foi compilado.
    static Target host()
    {
#if defined(_WIN32)
        return Target(TargetOS::Windows);
#elif defined(__APPLE__)
        return Target(TargetOS::MacOS);
#else
        return Target(TargetOS::Linux);
#endif
    }

private:
    TargetOS os_;
};
