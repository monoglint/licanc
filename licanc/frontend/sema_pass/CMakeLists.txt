add_library(frontend_sema_pass)

target_sources(frontend_sema_pass
    PUBLIC
        FILE_SET CXX_MODULES FILES
            sema_pass.ccm
            semantic_analyzer.ccm
            sym_registrar.ccm
    PRIVATE
        src/semantic_analyzer.cc
        src/sym_registrar.cc
)

target_link_libraries(frontend_sema_pass
    PRIVATE util frontend_sema
)