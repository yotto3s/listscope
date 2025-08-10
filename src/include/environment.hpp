#pragma once

#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <memory>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/StandardInstrumentations.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "JITCompiler.hpp"
#include "ast.hpp"

namespace ks
{
class CodeGenEnvironment
{
  public:
    std::unique_ptr<llvm::LLVMContext> context = nullptr;
    std::unique_ptr<llvm::IRBuilder<>> builder = nullptr;
    std::unique_ptr<llvm::Module> module = nullptr;
    std::unique_ptr<llvm::FunctionPassManager> function_pass_manager = nullptr;
    std::unique_ptr<llvm::LoopAnalysisManager> loop_analysis_manager = nullptr;
    std::unique_ptr<llvm::FunctionAnalysisManager> function_analysis_manager = nullptr;
    std::unique_ptr<llvm::CGSCCAnalysisManager> cgscc_analysis_manager = nullptr;
    std::unique_ptr<llvm::ModuleAnalysisManager> module_analysis_manager = nullptr;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> pass_instrumentation_callbacks = nullptr;
    std::unique_ptr<llvm::StandardInstrumentations> standard_instrumentations = nullptr;
    std::map<std::string, llvm::Value*> named_values{};
    std::map<std::string, std::unique_ptr<PrototypeAST>> function_prototypes{};

    explicit CodeGenEnvironment(llvm::DataLayout layout);

    static CodeGenEnvironment predefined_operators(llvm::DataLayout layout);

    void initialize_module_and_managers(llvm::DataLayout layout);

    llvm::orc::ResourceTrackerSP add_to_jit_compiler(JITCompiler& jit_compiler, bool resource_tracking = false);

    template <std::ranges::range Args> llvm::Function* gen_prototype(const std::string_view name, const Args& args)
    {
        const auto nargs = std::ranges::size(args);
        const auto doubles = std::vector<llvm::Type*>(nargs, llvm::Type::getDoubleTy(*this->context));
        const auto ty = llvm::FunctionType::get(llvm::Type::getDoubleTy(*this->context), std::move(doubles), false);
        const auto fun = llvm::Function::Create(ty, llvm::Function::ExternalLinkage, name, this->module.get());

        auto idx = std::size_t(0);
        for (auto& arg : fun->args())
        {
            arg.setName(args[idx++]);
        }

        const auto name_str = std::string(name);
        this->function_prototypes[name_str] =
            std::make_unique<PrototypeAST>(name_str, std::vector<std::string>(args.begin(), args.end()));

        return fun;
    }

    template <std::ranges::range Args>
    llvm::Function* gen_function(const std::string_view name, const Args& args,
                                 std::function<llvm::Value*(CodeGenEnvironment&)> body)
    {
        auto fun = this->module->getFunction(name);
        if (fun == nullptr)
        {
            fun = this->gen_prototype(name, args);
        }

        if (fun == nullptr)
        {
            return nullptr;
        }

        if (!fun->empty())
        {
            LogError(std::format("Function `{}` cannot be redefined.", name));
            return nullptr;
        }

        auto bb = llvm::BasicBlock::Create(*this->context, "entry", fun);
        this->builder->SetInsertPoint(bb);

        this->named_values.clear();
        for (auto& arg : fun->args())
        {
            this->named_values[std::string(arg.getName())] = &arg;
        }

        if (auto retval = body(*this))
        {
            this->builder->CreateRet(retval);
            llvm::verifyFunction(*fun);
            this->function_pass_manager->run(*fun, *this->function_analysis_manager);

            return fun;
        }

        fun->eraseFromParent();
        return nullptr;
    }

    llvm::Function* get_function(const std::string_view name);

  private:
    void register_operators();

    static llvm::Value* LogError(const std::string_view str)
    {
        std::cerr << str;
        return nullptr;
    }
};
} // namespace ks
