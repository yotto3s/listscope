add_llvm_executable(kaleidoscope
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ast.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/environment.cpp
)

set_property(TARGET kaleidoscope PROPERTY CXX_STANDARD 20)
target_include_directories(kaleidoscope
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # ${LLVM_INCLUDE_DIRS}
)
# target_link_libraries(kaleidoscope PRIVATE
# LLVMSupport
# LLVMCore
# LLVMPasses
# LLVMOrcJIT
# )
# target_link_directories(kaleidoscope PRIVATE ${LLVM_BUILD_LIBRARY_DIR})
# target_compile_definitions(kaleidoscope PRIVATE ${LLVM_DEFINITIONS})
