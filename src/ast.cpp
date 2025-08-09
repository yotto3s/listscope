#include "ast.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <string>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace ks {

static llvm::Value* LogErrorV(const std::string_view str) {
    std::cerr << str;
    return nullptr;
}

static std::optional<double> parse_number(const std::string& s) {
    auto iss = std::istringstream(s);
    auto d = double(0);
    if (iss >> std::noskipws >> d && iss.eof()) {
        return d;
    } else {
        return std::nullopt;
    }
}

llvm::Value* VariableExprAST::codegen(CodeGenEnvironment& env) {
    if (env.named_values.contains(this->name)) {
        // variable
        const auto v = env.named_values[this->name];
        if (v != nullptr) {
            return v;
        }
    } else if (auto d = parse_number(this->name); d.has_value()) {
        // if not variable, it is a number
        return llvm::ConstantFP::get(*env.context, llvm::APFloat(d.value()));
    }

    return LogErrorV(std::format("Unknown variable `{}`", this->name));
}

llvm::Value* CallExprAST::codegen(CodeGenEnvironment& env) {
    auto callee_fun = env.module->getFunction(this->callee);
    if (callee_fun == nullptr) {
        return LogErrorV(std::format("Unknown function `{}`", this->callee));
    }

    if (callee_fun->arg_size() != this->args.size()) {
        return LogErrorV(std::format(
            "function `{}` expects {} argments, passed {} argments",
            this->callee, callee_fun->arg_size(), this->args.size())
        );
    }

    auto args_v = std::vector<llvm::Value*>();
    args_v.reserve(this->args.size());

    std::ranges::transform(this->args, std::back_inserter(args_v), [&env] (auto& arg) {
        return arg->codegen(env);
    });

    if (std::ranges::any_of(args_v, [] (auto p) { return p == nullptr; })) {
        return nullptr;
    }

    return env.builder->CreateCall(callee_fun, args_v, "calltmp");
}
llvm::Function* PrototypeAST::codegen(CodeGenEnvironment& env) {
    return env.gen_prototype(this->name, this->args);
}

llvm::Function* FunctionAST::codegen(CodeGenEnvironment& env) {
    return env.gen_function(
        this->proto->get_name(),
        this->proto->get_args(),
        [this](auto& e) {
            return this->body->codegen(e);
        }
    );
}
}  // namespace ks
