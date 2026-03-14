add_library(frontend_parse_pass
    parser.hh
    parser.cc
)

target_include_directories(frontend_parse_pass PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_parse_pass
    PUBLIC util
)