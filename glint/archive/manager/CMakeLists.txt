add_library(manager)

target_sources(manager
    PUBLIC
        FILE_SET CXX_MODULES FILES
            manager.ixx
    PRIVATE
        src/manager.cxx
)

target_link_libraries(manager
    PRIVATE util frontend manager_types
)