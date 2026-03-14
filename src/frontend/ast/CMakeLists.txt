add_library(frontend_ast 
    ast_reader.hh
    ast_visitor.hh
    ast_writer.hh

    ast.hh
    src/ast.cc
)

target_include_directories(frontend_ast PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_ast
    PUBLIC util
)