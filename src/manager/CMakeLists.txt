add_library(manager 
    manager_types.hh
    manager.hh

    src/manager.cc
)

target_include_directories(manager PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
