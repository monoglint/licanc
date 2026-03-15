add_library(frontend_tok)

target_sources(frontend_tok
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            tok.ccm
    PRIVATE

)

target_link_libraries(frontend_tok
    PRIVATE util
)