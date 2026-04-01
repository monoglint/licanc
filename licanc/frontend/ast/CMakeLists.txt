add_library(frontend_ast)

target_sources(frontend_ast
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ast_builder.ixx
            ast_class.ixx
            ast_deserializer.ixx
            ast_nodes.ixx
            ast_printer.ixx
            ast_serializer.ixx
            ast_visitor.ixx
            ast.ixx
)

target_link_libraries(frontend_ast
    PRIVATE 
        util 
        frontend_tok
        driver_base
)