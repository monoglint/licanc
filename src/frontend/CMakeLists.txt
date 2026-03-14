add_subdirectory(tok)
add_subdirectory(ast)
add_subdirectory(sema)

add_subdirectory(lex_pass)
add_subdirectory(parse_pass)
add_subdirectory(sema_pass)

add_library(frontend INTERFACE)

target_link_libraries(frontend INTERFACE
    frontend_ast
    frontend_tok
    frontend_sema
    frontend_lex_pass
    frontend_parse_pass
    frontend_sema_pass
)