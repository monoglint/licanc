add_library(frontend_sema_pass 
    full_passer.hh
    semantic_analyzer.hh
    sym_registrar.hh

    src/full_passer.cc
    src/semantic_analyzer.cc
    src/sym_registrar.cc
)

target_include_directories(frontend_sema_pass PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_sema_pass
    PUBLIC util
)