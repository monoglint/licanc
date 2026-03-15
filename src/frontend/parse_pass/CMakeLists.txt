add_library(frontend_parse_pass)

target_sources(frontend_parse_pass
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            parser.ccm
    PRIVATE
        src/parser.cc
)

target_link_libraries(frontend_parse_pass
    PUBLIC util
)