add_library(frontend_ast)

target_sources(frontend_ast
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            ast.ccm
            ast_reader.ccm
            ast_visitor.ccm
            ast_writer.ccm
    PRIVATE
        src/ast.cc
)

target_link_libraries(frontend_ast
    PUBLIC util
)