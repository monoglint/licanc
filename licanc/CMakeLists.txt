add_subdirectory(build_system)
add_subdirectory(frontend)
add_subdirectory(manager)
add_subdirectory(manager_types)
add_subdirectory(util)

add_library(licanc_library INTERFACE
    build_system
    frontend
    manager
    manager_types
    util
)

add_executable(licanc
    main.cc
)

target_link_libraries(licanc PRIVATE licanc_library)