add_library(build_system)

target_sources(build_system
    PUBLIC
        FILE_SET CXX_MODULES FILES
            build_system.ccm
    PRIVATE
        src/build_system.cc
)

target_link_libraries(build_system
    PRIVATE util manager
)