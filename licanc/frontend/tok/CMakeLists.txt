add_library(frontend_tok)

target_sources(frontend_tok
    PUBLIC
        FILE_SET CXX_MODULES FILES
            tok.ccm
    PRIVATE

)

target_link_libraries(frontend_tok
    PRIVATE util
)