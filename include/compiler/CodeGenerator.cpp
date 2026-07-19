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

CodeGenerator::CodeGenerator(const std::string& filename)
{
  output.open(filename);
  if (!output.is_open()) {
    throw std::runtime_error("Cannot open output file: " + filename);
  }
}

std::string CodeGenerator::labelFor(const std::string& name) const
{
  return name;
}

void CodeGenerator::generate(const Program& program)
{
  dataSection.clear();
  textSection.clear();
  variableTypes.clear();
  stringCounter = 0;

  // Prelude for text section
  textSection.push_back(".globl main");
  textSection.push_back(".type main, @function");
  textSection.push_back("main:");
  textSection.push_back("    pushq %rbp");
  textSection.push_back("    movq %rsp, %rbp");

  // Walk AST
  for (const auto& stmt : program.statements) {
    generateNode(stmt.get());
  }

  // Epilogue
  textSection.push_back("    movl $0, %eax");
  textSection.push_back("    popq %rbp");
  textSection.push_back("    ret");

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

  // Other node types currently not handled
}

void CodeGenerator::generateVariable(const VariableDeclaration* node)
{
  std::string label = node->name;
  if (label.empty()) {
    label = "var_" + std::to_string(++stringCounter);
  }
  variableTypes[label] = node->type;

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
  }

  // no initializer: reserve zero
  std::ostringstream ss;
  if (node->type == "int") {
    ss << label << ": .long 0";
  } else {
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
      if (!containsLabel(dataSection, "__fmt_int"))
        dataSection.push_back("__fmt_int: .asciz \"%d\"");
      textSection.push_back("    leaq __fmt_int(%rip), %rdi");
      textSection.push_back("    leaq " + labelFor(ident->name) + "(%rip), %rsi");
      textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call scanf");
      return;
    }
  }
}

void CodeGenerator::emitPrintExpression(const Expression* expression)
{
  if (!containsLabel(dataSection, "__fmt_int")) {
    dataSection.push_back("__fmt_int: .asciz \"%d\\n\"");
  }

  if (!containsLabel(dataSection, "__fmt_str")) {
    dataSection.push_back("__fmt_str: .asciz \"%s\\n\"");
  }

  if (const auto* number = dynamic_cast<const NumberLiteral*>(expression)) {
    textSection.push_back("    movl $" + std::to_string(number->value) + ", %esi");
    textSection.push_back("    leaq __fmt_int(%rip), %rdi");
    textSection.push_back("    movl $0, %eax");
    textSection.push_back("    call printf");
    return;
  }

  if (const auto* text = dynamic_cast<const stringLiteral*>(expression)) {
    const std::string label = ".LC" + std::to_string(++stringCounter);
    std::ostringstream ss;
    ss << label << ": .asciz \"" << escapeString(text->value) << "\"";
    dataSection.push_back(ss.str());
    textSection.push_back("    leaq " + label + "(%rip), %rdi");
    textSection.push_back("    call puts");
    return;
  }

  if (const auto* ident = dynamic_cast<const Identifer*>(expression)) {
    const auto typeIt = variableTypes.find(ident->name);
    if (typeIt != variableTypes.end() && typeIt->second == "int") {
      textSection.push_back("    movl " + labelFor(ident->name) + "(%rip), %esi");
      textSection.push_back("    leaq __fmt_int(%rip), %rdi");
      textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call printf");
      return;
    }

    textSection.push_back("    leaq " + labelFor(ident->name) + "(%rip), %rdi");
    textSection.push_back("    call puts");
  }
}

void CodeGenerator::writeFile()
{
  // Write data section
  if (!dataSection.empty()) {
    output << "    .section .data\n";
    for (const auto &line : dataSection) {
      output << line << "\n";
    }
    output << "\n";
  }

  // Extern declarations
  output << "    .extern printf\n";
  output << "    .extern puts\n";
  output << "\n";

  // Write text section
  output << "    .section .text\n";
  for (const auto &line : textSection) {
    output << line << "\n";
  }

  output << "    .extern scanf\n";
  output << "    .section .note.GNU-stack,\"\",@progbits\n";

  output.close();
}
