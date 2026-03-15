add_library(manager_types)

target_sources(manager_types
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            manager_types.ccm
    PRIVATE
)

target_link_libraries(manager_types
    PUBLIC util
)