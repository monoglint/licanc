add_library(frontend_sema_pass)

target_sources(frontend_sema_pass
    PUBLIC
        FILE_SET CXX_MODULES FILES
            sema_pass.ixx
            semantic_analyzer.ixx
            sym_registrar.ixx
    PRIVATE
        src/semantic_analyzer.cxx
        src/sym_registrar.cxx
)

target_link_libraries(frontend_sema_pass
    PRIVATE util frontend_sema
)