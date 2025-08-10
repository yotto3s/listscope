#pragma once

#include <memory>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/Mangling.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace ks
{

class JITCompiler
{
  private:
    std::unique_ptr<llvm::orc::ExecutionSession> session;
    llvm::DataLayout layout;
    llvm::orc::MangleAndInterner mangle;
    llvm::orc::RTDyldObjectLinkingLayer object_layer;
    llvm::orc::IRCompileLayer compile_layer;
    llvm::orc::JITDylib& main_dylib;

  public:
    JITCompiler(std::unique_ptr<llvm::orc::ExecutionSession> _session, llvm::orc::JITTargetMachineBuilder builder,
                llvm::DataLayout _layout)
        : session(std::move(_session)), layout(std::move(_layout)), mangle(*this->session, this->layout),
          object_layer(*this->session, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
          compile_layer(*this->session, this->object_layer,
                        std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(builder))),
          main_dylib(this->session->createBareJITDylib("<main>"))
    {
        this->main_dylib.addGenerator(llvm::cantFail(
            llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(this->layout.getGlobalPrefix())));
        if (builder.getTargetTriple().isOSBinFormatCOFF())
        {
            this->object_layer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            this->object_layer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    ~JITCompiler()
    {
        if (auto err = this->session->endSession())
        {
            this->session->reportError(std::move(err));
        }
    }

    static llvm::Expected<std::unique_ptr<JITCompiler>> create()
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetAsmPrinter();
        auto epc = llvm::orc::SelfExecutorProcessControl::Create();
        if (!epc)
        {
            return epc.takeError();
        }

        auto session = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));
        auto builder = llvm::orc::JITTargetMachineBuilder(session->getExecutorProcessControl().getTargetTriple());

        auto layout = builder.getDefaultDataLayoutForTarget();
        if (!layout)
        {
            return layout.takeError();
        }
        return std::make_unique<JITCompiler>(std::move(session), std::move(builder), std::move(*layout));
    }

    llvm::Error add_module(llvm::orc::ThreadSafeModule module, llvm::orc::ResourceTrackerSP resource_tracker = nullptr)
    {
        if (!resource_tracker)
        {
            resource_tracker = this->main_dylib.getDefaultResourceTracker();
        }

        return this->compile_layer.add(std::move(resource_tracker), std::move(module));
    }

    llvm::Expected<llvm::orc::ExecutorSymbolDef> lookup(llvm::StringRef name)
    {
        return this->session->lookup({&this->main_dylib},
                                     llvm::orc::MangleAndInterner(*this->session, this->layout)(name.str()));
    }

    const llvm::DataLayout& get_data_layout() const
    {
        return this->layout;
    }

    llvm::orc::JITDylib& get_main_jit_dylib()
    {
        return this->main_dylib;
    }
};
} // namespace ks
