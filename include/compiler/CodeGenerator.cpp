#include "CodeGenerator.hpp"

#include <stdexcept>
#include <sstream>
#include <algorithm>

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

void CodeGenerator::generate(const Program& program)
{
  dataSection.clear();
  textSection.clear();
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
  // Support a simple built-in: print(x)
  // Support builtins: print/sayf and scan
  if (node->functionName == "print" || node->functionName == "sayf") {
    if (node->arguments.empty()) return;

    const Expression* arg = node->arguments[0].get();

    // Ensure format strings exist
    if (!containsLabel(dataSection, "__fmt_int"))
      dataSection.push_back("__fmt_int: .asciz \"%d\\n\"");
    if (!containsLabel(dataSection, "__fmt_str"))
      dataSection.push_back("__fmt_str: .asciz \"%s\\n\"");

    if (const auto *num = dynamic_cast<const NumberLiteral*>(arg)) {
      std::ostringstream ss;
      ss << "    movl $" << num->value << ", %esi";
      textSection.push_back(ss.str());
      textSection.push_back("    leaq __fmt_int(%rip), %rdi");
      textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call printf");
      return;
    }

    if (const auto *str = dynamic_cast<const stringLiteral*>(arg)) {
      std::string lbl = ".LC" + std::to_string(++stringCounter);
      std::string esc = escapeString(str->value);
      std::ostringstream ds;
      ds << lbl << ": .asciz \"" << esc << "\"";
      dataSection.push_back(ds.str());

      textSection.push_back("    leaq " + lbl + "(%rip), %rdi");
      textSection.push_back("    call puts");
      return;
    }

    if (const auto *ident = dynamic_cast<const Identifer*>(arg)) {
      // treat identifier as string pointer for sayf/print
      textSection.push_back("    leaq " + ident->name + "(%rip), %rdi");
      textSection.push_back("    call puts");
      return;
    }
  }

  if (node->functionName == "scan") {
    // scan(identifier) -> scanf("%d", &identifier)
    if (node->arguments.empty()) return;
    const Expression* arg = node->arguments[0].get();
    if (const auto *ident = dynamic_cast<const Identifer*>(arg)) {
      if (!containsLabel(dataSection, "__fmt_int"))
        dataSection.push_back("__fmt_int: .asciz \"%d\"");
      textSection.push_back("    leaq __fmt_int(%rip), %rdi");
      textSection.push_back("    leaq " + ident->name + "(%rip), %rsi");
      textSection.push_back("    movl $0, %eax");
      textSection.push_back("    call scanf");
      return;
    }
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

  output.close();
}
