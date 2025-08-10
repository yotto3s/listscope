#include <format>
#include <iostream>
#include <llvm/Support/Error.h>
#include <variant>

#include "JITCompiler.hpp"
#include "ast.hpp"
#include "environment.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main()
{
    static llvm::ExitOnError exit_on_error;
    auto lexer = ks::Lexer(std::cin);
    auto parser = ks::Parser(std::move(lexer));
    auto jit_compiler = ks::JITCompiler::create();
    auto p_jit_compiler = jit_compiler ? std::move(jit_compiler.get()) : nullptr;
    if (!p_jit_compiler)
    {
        std::cout << llvm::toString(jit_compiler.takeError());
        return 0;
    }
    auto env = ks::CodeGenEnvironment::predefined_operators(p_jit_compiler->get_data_layout());
    env.add_to_jit_compiler(*p_jit_compiler);
    while (true)
    {
        std::cout << "> ";
        auto result = parser.parse_top_level();
        if (!result.has_value())
        {
            break;
        }
        auto p = std::move(result.value());
        auto fn_ir = std::visit([&env](auto& x) { return x->codegen(env); }, p);

        if (!fn_ir)
        {
            break;
        }

        if (std::holds_alternative<std::unique_ptr<ks::FunctionAST>>(p))
        {
            auto& fun_ast = std::get<std::unique_ptr<ks::FunctionAST>>(p);
            const auto is_top_expr = fun_ast->is_top_level_expression();
            auto resource_tracker = env.add_to_jit_compiler(*p_jit_compiler, is_top_expr);
            if (is_top_expr)
            {
                auto ExprSymbol = exit_on_error(p_jit_compiler->lookup(fun_ast->get_name()));

                // Get the symbol's address and cast it to the right type (takes no
                // arguments, returns a double) so we can call it as a native function.
                auto FP = ExprSymbol.getAddress().toPtr<double (*)()>();
                std::cout << std::format("Evaluated to {}\n", FP());
                exit_on_error(resource_tracker->remove());
            }
        }
    }

    env.module->print(llvm::errs(), nullptr);
    return 0;
}
