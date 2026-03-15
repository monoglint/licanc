add_library(manager)

target_sources(manager
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            manager.ccm
    PRIVATE
        src/manager.cc
)

target_link_libraries(manager
    PRIVATE util
)