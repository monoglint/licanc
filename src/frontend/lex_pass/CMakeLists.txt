add_library(frontend_lex_pass 
    lexer.hh
    src/lexer.cc
)

target_include_directories(frontend_lex_pass PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_lex_pass
    PUBLIC util
)