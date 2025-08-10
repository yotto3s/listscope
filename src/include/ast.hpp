#pragma once

#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace ks
{
class CodeGenEnvironment;
class ExprAST
{
  public:
    virtual ~ExprAST() = default;
    virtual std::string to_string() const = 0;
    virtual llvm::Value* codegen(CodeGenEnvironment& env) = 0;
};

class VariableExprAST : public ExprAST
{
    std::string name;

  public:
    VariableExprAST(const std::string& _name) : name(_name)
    {
    }
    virtual std::string to_string() const override
    {
        return std::format("Variable({})", this->name);
    }
    virtual llvm::Value* codegen(CodeGenEnvironment& env) override;
};

class CallExprAST : public ExprAST
{
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

  public:
    CallExprAST(std::string _callee, std::vector<std::unique_ptr<ExprAST>> _args)
        : callee(std::move(_callee)), args(std::move(_args))
    {
    }

    virtual std::string to_string() const override
    {
        auto ss = std::stringstream();
        for (const auto& arg : this->args)
        {
            ss << arg->to_string() << ',';
        }
        return std::format("CALL(fun: {}, args: [{}])", this->callee, ss.str());
    }

    virtual llvm::Value* codegen(CodeGenEnvironment& env) override;
};

class PrototypeAST
{
    std::string name;
    std::vector<std::string> args;

  public:
    PrototypeAST(const std::string _name, std::vector<std::string> _args) : name(_name), args(std::move(_args))
    {
    }
    const std::string& get_name() const
    {
        return this->name;
    }
    const std::vector<std::string> get_args() const
    {
        return this->args;
    }
    llvm::Function* codegen(CodeGenEnvironment& env);
    std::string to_string() const
    {
        auto ss = std::stringstream();
        for (const auto& arg : this->args)
        {
            ss << arg << ',';
        }
        return std::format("Prototype(name: {}, args [{}])", this->name, ss.str());
    }
};

class FunctionAST
{
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;
    bool is_top_level;

  public:
    FunctionAST(std::unique_ptr<PrototypeAST> _proto, std::unique_ptr<ExprAST> _body, bool _is_top_level = false)
        : proto(std::move(_proto)), body(std::move(_body)), is_top_level(_is_top_level)
    {
    }
    llvm::Function* codegen(CodeGenEnvironment& env);

    std::string to_string() const
    {
        return std::format("Function(proto: {}\n body: {})", this->proto->to_string(), this->body->to_string());
    }

    bool is_top_level_expression() const
    {
        return this->is_top_level;
    }

    std::string_view get_name() const
    {
        return this->proto->get_name();
    }
};
} // namespace ks
