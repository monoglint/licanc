add_library(frontend_tok 
    tok.hh
)

target_include_directories(frontend_tok PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_tok
    PUBLIC util
)