#include "environment.hpp"

#include <memory>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace ks
{

CodeGenEnvironment::CodeGenEnvironment(llvm::DataLayout layout)
{
    this->initialize_module_and_managers(layout);
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

    this->register_operators();
}

void CodeGenEnvironment::register_operators()
{
    constexpr auto args = std::array<std::string_view, 2>{"x", "y"};
    const auto add = this->gen_function("+", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFAdd(lhs, rhs, "addtmp");
    });
    assert(add);
    const auto sub = this->gen_function("-", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFSub(lhs, rhs, "subtmp");
    });
    assert(sub);
    const auto mul = this->gen_function("*", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFMul(lhs, rhs, "multmp");
    });
    assert(mul);
    const auto div = this->gen_function("/", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        return env.builder->CreateFDiv(lhs, rhs, "divtmp");
    });
    assert(div);
    const auto gt = this->gen_function("<", args, [&args](auto& env) {
        const auto lhs = env.named_values[std::string(args[0])];
        const auto rhs = env.named_values[std::string(args[1])];
        auto ui = env.builder->CreateFCmpULT(lhs, rhs, "cmptmp");
        return env.builder->CreateUIToFP(ui, llvm::Type::getDoubleTy(*env.context), "booltmp");
    });
    assert(gt);
}
} // namespace ks
