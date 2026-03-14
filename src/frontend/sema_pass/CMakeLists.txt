add_library(frontend_sema_pass
    full_passer.hh
    semantic_analyzer.hh
    sym_registrar.hh

    full_passer.cc
    semantic_analyzer.cc
    sym_registrar.cc
)

target_include_directories(frontend_sema_pass PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_sema_pass
    PUBLIC util
)