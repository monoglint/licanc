add_library(frontend_lex_pass)

target_sources(frontend_lex_pass
    PUBLIC
        FILE_SET CXX_MODULES FILES
            lexer.ixx
    PRIVATE
        src/lexer.cxx
)

target_link_libraries(frontend_lex_pass
    PRIVATE util
)