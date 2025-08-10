#include "environment.hpp"

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/Support/Error.h>
#include <memory>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace ks
{

CodeGenEnvironment::CodeGenEnvironment(llvm::DataLayout layout)
{
    this->initialize_module_and_managers(layout);
}

CodeGenEnvironment CodeGenEnvironment::predefined_operators(llvm::DataLayout layout)
{
    CodeGenEnvironment env = CodeGenEnvironment(layout);
    env.register_operators();
    return env;
}

void CodeGenEnvironment::initialize_module_and_managers(llvm::DataLayout layout)
{
    this->context = std::make_unique<llvm::LLVMContext>();
    this->builder = std::make_unique<llvm::IRBuilder<>>(*this->context);
    this->module = std::make_unique<llvm::Module>("my cool jit", *this->context);
    this->module->setDataLayout(layout);

    this->function_pass_manager = std::make_unique<llvm::FunctionPassManager>();
    this->loop_analysis_manager = std::make_unique<llvm::LoopAnalysisManager>();
    this->function_analysis_manager = std::make_unique<llvm::FunctionAnalysisManager>();
    this->cgscc_analysis_manager = std::make_unique<llvm::CGSCCAnalysisManager>();
    this->module_analysis_manager = std::make_unique<llvm::ModuleAnalysisManager>();
    this->pass_instrumentation_callbacks = std::make_unique<llvm::PassInstrumentationCallbacks>();
    this->standard_instrumentations = std::make_unique<llvm::StandardInstrumentations>(*this->context, true);

    this->standard_instrumentations->registerCallbacks(*this->pass_instrumentation_callbacks,
                                                       this->module_analysis_manager.get());

    this->function_pass_manager->addPass(llvm::InstCombinePass());
    this->function_pass_manager->addPass(llvm::ReassociatePass());
    this->function_pass_manager->addPass(llvm::GVNPass());
    this->function_pass_manager->addPass(llvm::SimplifyCFGPass());

    auto pass_builder = llvm::PassBuilder();
    pass_builder.registerModuleAnalyses(*this->module_analysis_manager);
    pass_builder.registerFunctionAnalyses(*this->function_analysis_manager);
    pass_builder.crossRegisterProxies(*this->loop_analysis_manager, *this->function_analysis_manager,
                                      *this->cgscc_analysis_manager, *this->module_analysis_manager);
}

llvm::orc::ResourceTrackerSP CodeGenEnvironment::add_to_jit_compiler(JITCompiler& jit_compiler, bool resource_tracking)
{
    static auto exit_on_error = llvm::ExitOnError();
    auto resource_tracker = resource_tracking ? jit_compiler.get_main_jit_dylib().createResourceTracker() : nullptr;
    auto thread_safe_module = llvm::orc::ThreadSafeModule(std::move(this->module), std::move(this->context));
    exit_on_error(jit_compiler.add_module(std::move(thread_safe_module), resource_tracker));
    this->initialize_module_and_managers(jit_compiler.get_data_layout());

    return resource_tracker;
}

llvm::Function* CodeGenEnvironment::get_function(const std::string_view name)
{
    if (const auto fun = this->module->getFunction(name))
    {
        return fun;
    }
    const auto fi = this->function_prototypes.find(std::string(name));
    if (fi != this->function_prototypes.end())
    {
        return fi->second->codegen(*this);
    }

    LogError(std::format("Function `{}` not found.", name));
    return nullptr;
}

void CodeGenEnvironment::register_operators()
{
    constexpr auto args = std::array<std::string_view, 2>{"x", "y"};
    std::ignore = this->gen_function("+", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFAdd(lhs, rhs, "addtmp");
    });
    std::ignore = this->gen_function("-", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFSub(lhs, rhs, "subtmp");
    });
    std::ignore = this->gen_function("*", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFMul(lhs, rhs, "multmp");
    });
    std::ignore = this->gen_function("/", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFDiv(lhs, rhs, "divtmp");
    });
    std::ignore = this->gen_function("<", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        auto ui = env.builder->CreateFCmpULT(lhs, rhs, "cmptmp");
        return env.builder->CreateUIToFP(ui, llvm::Type::getDoubleTy(*env.context), "booltmp");
    });
}
} // namespace ks
