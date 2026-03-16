add_library(frontend_sema)

target_sources(frontend_sema
    PUBLIC
        FILE_SET CXX_MODULES FILES
            sema.ccm
            sym.ccm
            resolved_type.ccm
            ct_expr.ccm
)

target_link_libraries(frontend_sema
    PRIVATE 
        util
        frontend_ast
)