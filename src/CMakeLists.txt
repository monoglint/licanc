add_subdirectory(frontend)
add_subdirectory(manager)
add_subdirectory(util)

add_executable(licanc
    main.cc
)

target_link_libraries(licanc PUBLIC
    frontend
    manager
    util
)