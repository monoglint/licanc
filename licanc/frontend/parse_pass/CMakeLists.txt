add_library(frontend_parse_pass)

target_sources(frontend_parse_pass
    PUBLIC
        FILE_SET CXX_MODULES FILES
            parser.ixx
    PRIVATE
        src/parser.cxx
)

target_link_libraries(frontend_parse_pass
    PRIVATE util frontend_ast
)