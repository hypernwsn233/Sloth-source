#include "CodeGenerator.hpp"

#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {
  static std::string escapeString(const std::string &s) {
    std::string out;
    for (char c : s) {
      if (c == '\\') out += "\\\\";
      else if (c == '"') out += "\\\"";
      else if (c == '\n') out += "\\n";
      else out.push_back(c);
    }
    return out;
  }

  static bool containsLabel(const std::vector<std::string>& data, const std::string& label) {
    for (const auto &line : data) {
      if (line.rfind(label + ":", 0) == 0) return true;
    }
    return false;
  }
}

CodeGenerator::CodeGenerator(const std::string& filename, Target target, int optLevel)
  : outputPath(filename), target(target), optLevel(optLevel)
{
  output.open(filename);
  if (!output.is_open()) {
    throw std::runtime_error("Cannot open output file: " + filename);
  }
}

std::string CodeGenerator::labelFor(const std::string& name) const
{
  // Símbolos de dados também são decorados no Mach-O (macOS).
  return target.mangle(name);
}

void CodeGenerator::generate(const Program& program)
{
  dataSection.clear();
  textSection.clear();
  variableTypes.clear();
  stringCounter = 0;

  // Prelude do bloco .text — entrypoint decorado por SO.
  const std::string entry = target.entry();
  textSection.push_back(".globl " + entry);
  if (target.os() == TargetOS::Linux) {
    textSection.push_back(".type " + entry + ", @function");
  }
  textSection.push_back(entry + ":");
  textSection.push_back("    pushq %rbp");
  textSection.push_back("    movq %rsp, %rbp");

  // Win64 exige 32 bytes de "shadow space" antes de qualquer call; mantemos
  // a pilha alinhada a 16 bytes reservando esse espaço no prólogo.
  if (target.shadowSpace() > 0) {
    textSection.push_back("    subq $" + std::to_string(target.shadowSpace()) + ", %rsp");
  }

  // Percorre a AST.
  for (const auto& stmt : program.statements) {
    generateNode(stmt.get());
  }

  // Epílogo.
  if (target.shadowSpace() > 0) {
    textSection.push_back("    addq $" + std::to_string(target.shadowSpace()) + ", %rsp");
  }
  textSection.push_back("    movl $0, %eax");
  textSection.push_back("    popq %rbp");
  textSection.push_back("    ret");

  // Otimização peephole sobre a seção .text.
  AssemblyOptimizer optimizer(optLevel);
  lastStats = optimizer.optimize(textSection);

  writeFile();
}

void CodeGenerator::generateNode(const ASTnode* node)
{
  if (const auto *var = dynamic_cast<const VariableDeclaration*>(node)) {
    generateVariable(var);
    return;
  }

  if (const auto *call = dynamic_cast<const FunctionCall*>(node)) {
    generateFunctionCall(call);
    return;
  }

  // Outros tipos de nó ainda não tratados.
}

void CodeGenerator::generateVariable(const VariableDeclaration* node)
{
  std::string label = labelFor(node->name);
  if (node->name.empty()) {
    label = labelFor("var_" + std::to_string(++stringCounter));
  }
  variableTypes[node->name] = node->type;

  if (node->value) {
    if (const auto *num = dynamic_cast<const NumberLiteral*>(node->value.get())) {
      std::ostringstream ss;
      if (node->type == "int") {
        ss << label << ": .long " << num->value;
      } else {
        ss << label << ": .quad " << num->value;
      }
      if (!containsLabel(dataSection, label)) dataSection.push_back(ss.str());
      return;
    }

    if (const auto *str = dynamic_cast<const stringLiteral*>(node->value.get())) {
      std::string esc = escapeString(str->value);
      std::ostringstream ss;
      ss << label << ": .asciz \"" << esc << "\"";
      if (!containsLabel(dataSection, label)) dataSection.push_back(ss.str());
      return;
    }

    if (const auto *boolean = dynamic_cast<const BooleanLiteral*>(node->value.get())) {
      // Um bool ocupa 1 byte: 1 = true, 0 = false.
      std::ostringstream ss;
      ss << label << ": .byte " << (boolean->value ? 1 : 0);
      if (!containsLabel(dataSection, label)) dataSection.push_back(ss.str());
      return;
    }
  }

  // Sem inicializador: reserva zero, no tamanho apropriado ao tipo.
  std::ostringstream ss;
  if (node->type == "int") {
    ss << label << ": .long 0";
  }
  else if (node->type == "bool") {
    ss << label << ": .byte 0";
  }
  else {
    ss << label << ": .quad 0";
  }
  if (!containsLabel(dataSection, label)) dataSection.push_back(ss.str());
}

void CodeGenerator::generateFunctionCall(const FunctionCall* node)
{
  if (node->functionName == "print" || node->functionName == "sayf") {
    if (node->arguments.empty()) return;
    emitPrintExpression(node->arguments[0].get());
    return;
  }

  if (node->functionName == "scan") {
    // scan(identifier) -> scanf("%d", &identifier)
    if (node->arguments.empty()) return;
    const Expression* arg = node->arguments[0].get();
    if (const auto *ident = dynamic_cast<const Identifer*>(arg)) {
      const std::string fmt = "__fmt_int_scan";
      if (!containsLabel(dataSection, fmt))
        dataSection.push_back(fmt + ": .asciz \"%d\"");

      // arg0 = ponteiro para a string de formato; arg1 = endereço da variável.
      textSection.push_back("    leaq " + fmt + "(%rip), " + target.argReg64(0));
      textSection.push_back("    leaq " + labelFor(ident->name) + "(%rip), " + target.argReg64(1));
      if (target.variadicNeedsAL())
        textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call " + target.mangle("scanf"));
      return;
    }
  }
}

void CodeGenerator::ensureBoolStrings()
{
  if (!containsLabel(dataSection, "__bool_true"))
    dataSection.push_back("__bool_true: .asciz \"true\"");
  if (!containsLabel(dataSection, "__bool_false"))
    dataSection.push_back("__bool_false: .asciz \"false\"");
}

void CodeGenerator::emitPrintExpression(const Expression* expression)
{
  const std::string fmt = "__fmt_int_print";
  if (!containsLabel(dataSection, fmt)) {
    dataSection.push_back(fmt + ": .asciz \"%d\\n\"");
  }

  if (const auto* number = dynamic_cast<const NumberLiteral*>(expression)) {
    // arg1 (valor) em 32 bits, arg0 (formato) como ponteiro.
    textSection.push_back("    movl $" + std::to_string(number->value) + ", " + target.argReg32(1));
    textSection.push_back("    leaq " + fmt + "(%rip), " + target.argReg64(0));
    if (target.variadicNeedsAL())
      textSection.push_back("    movl $0, %eax");
    textSection.push_back("    call " + target.mangle("printf"));
    return;
  }
  
  if (const auto* text = dynamic_cast<const stringLiteral*>(expression)) {
    const std::string label = ".LC" + std::to_string(++stringCounter);
    std::ostringstream ss;
    ss << label << ": .asciz \"" << escapeString(text->value) << "\"";
    dataSection.push_back(ss.str());
    textSection.push_back("    leaq " + label + "(%rip), " + target.argReg64(0));
    textSection.push_back("    call " + target.mangle("puts"));
    return;
  }

  if (const auto* boolean = dynamic_cast<const BooleanLiteral*>(expression)) {
    // Um literal booleano tem valor conhecido em tempo de compilacao:
    // imprimimos direto a string "true" ou "false".
    ensureBoolStrings();
    const std::string label = boolean->value ? "__bool_true" : "__bool_false";
    textSection.push_back("    leaq " + label + "(%rip), " + target.argReg64(0));
    textSection.push_back("    call " + target.mangle("puts"));
    return;
  }

  if (const auto* ident = dynamic_cast<const Identifer*>(expression)) {
    const auto typeIt = variableTypes.find(ident->name);
    if (typeIt != variableTypes.end() && typeIt->second == "int") {
      textSection.push_back("    movl " + labelFor(ident->name) + "(%rip), " + target.argReg32(1));
      textSection.push_back("    leaq " + fmt + "(%rip), " + target.argReg64(0));
      if (target.variadicNeedsAL())
        textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call " + target.mangle("printf"));
      return;
    }

    if (typeIt != variableTypes.end() && typeIt->second == "bool") {
      // Valor so conhecido em runtime: seleciona "true"/"false" com cmov.
      // Le o byte do bool, escolhe o ponteiro da string e chama puts.
      ensureBoolStrings();
      textSection.push_back("    movzbl " + labelFor(ident->name) + "(%rip), %eax");
      textSection.push_back("    leaq __bool_false(%rip), %r10");
      textSection.push_back("    leaq __bool_true(%rip), %r11");
      textSection.push_back("    testl %eax, %eax");
      textSection.push_back("    cmovne %r11, %r10");
      textSection.push_back("    movq %r10, " + target.argReg64(0));
      textSection.push_back("    call " + target.mangle("puts"));
      return;
    }

    textSection.push_back("    leaq " + labelFor(ident->name) + "(%rip), " + target.argReg64(0));
    textSection.push_back("    call " + target.mangle("puts"));
  }
}

void CodeGenerator::assemble(std::vector<std::string>& out) const
{
  out.clear();

  // Cabeçalho informativo.
  out.push_back("# ============================================================");
  out.push_back("# Gerado pelo compilador Unbit");
  out.push_back("# Alvo: " + target.name());
  out.push_back("# ============================================================");
  out.push_back("");

  // Seção de dados.
  if (!dataSection.empty()) {
    out.push_back(target.dataSectionDirective());
    for (const auto &line : dataSection) out.push_back(line);
    out.push_back("");
  }

  // Declarações externas (com decoração de símbolo por SO).
  out.push_back("    .extern " + target.mangle("printf"));
  out.push_back("    .extern " + target.mangle("puts"));
  out.push_back("    .extern " + target.mangle("scanf"));
  out.push_back("");

  // Seção de código.
  out.push_back(target.textSectionDirective());
  for (const auto &line : textSection) out.push_back(line);

  // Nota de pilha não-executável (apenas ELF/Linux).
  if (target.emitGNUStackNote()) {
    out.push_back("    .section .note.GNU-stack,\"\",@progbits");
  }
}

std::string CodeGenerator::assemblyText() const
{
  std::vector<std::string> lines;
  assemble(lines);
  std::ostringstream ss;
  for (const auto& l : lines) ss << l << "\n";
  return ss.str();
}

void CodeGenerator::writeFile()
{
  std::vector<std::string> lines;
  assemble(lines);
  for (const auto& l : lines) output << l << "\n";
  output.close();
}
