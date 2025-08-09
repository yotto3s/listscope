#include <format>
#include <iostream>
#include <variant>

#include "ast.hpp"
#include "JITCompiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main() {
    auto lexer = ks::Lexer(std::cin);
    auto parser = ks::Parser(std::move(lexer));
    auto jit_compiler = ks::JITCompiler::create();
    auto p_jit_compiler = jit_compiler ? std::move(jit_compiler.get()) : nullptr;
    if (!p_jit_compiler) {
        std::cout << llvm::toString(jit_compiler.takeError());
        return 0;
    }
    auto env = ks::CodeGenEnvironment(p_jit_compiler->get_data_layout());
    while (true) {
        std::cout << "> ";
        auto result = parser.parse_top_level();
        if (!result.has_value()) { break; }
        auto p = std::move(result.value());
        auto fn_ir = std::visit([&env](auto& x) {
            return x->codegen(env);
        }, p);

        if (!fn_ir) { break; }
    }

    env.module->print(llvm::errs(), nullptr);
    return 0;
}
