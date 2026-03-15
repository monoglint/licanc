add_library(frontend_ast)

target_sources(frontend_ast
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ast.ccm
            ast_nodes.ccm
            ast_reader.ccm
            ast_visitor.ccm
            ast_writer.ccm
)

target_link_libraries(frontend_ast
    PRIVATE util frontend_tok manager_types
)