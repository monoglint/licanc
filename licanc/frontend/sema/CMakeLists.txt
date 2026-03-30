add_library(frontend_sema)

target_sources(frontend_sema
    PUBLIC
        FILE_SET CXX_MODULES FILES
            sema.ixx
            syms.ixx
            syms_fwd.ixx
            sym_table_class.ixx
            resolved_type.ixx
            ct_expr.ixx
)

target_link_libraries(frontend_sema
    PRIVATE 
        util
        frontend_ast
)