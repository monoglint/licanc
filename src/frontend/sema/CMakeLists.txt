add_library(frontend_sema)

target_sources(frontend_sema
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            sema.ccm
            sym.ccm
            resolved_type.ccm
    PRIVATE
        src/sym.cc
)

target_link_libraries(frontend_sema
    PUBLIC util
)