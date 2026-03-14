add_library(frontend_sema 
    sym.hh
    src/sym.cc
)

target_include_directories(frontend_sema PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(frontend_sema
    PUBLIC util
)