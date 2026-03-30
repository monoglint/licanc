add_library(frontend_ast)

target_sources(frontend_ast
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ast.ixx
            ast_nodes.ixx
            ast_class.ixx
            ast_reader.ixx
            ast_visitor.ixx
            ast_writer.ixx
)

target_link_libraries(frontend_ast
    PRIVATE 
        util 
        frontend_tok
        driver_base
)