add_subdirectory(package)
add_subdirectory(frontend)
add_subdirectory(util)

add_library(licanc_library INTERFACE)

target_link_libraries(licanc_library INTERFACE
    package
    frontend
    util
)

add_executable(licanc main.cxx)

target_link_libraries(licanc PRIVATE licanc_library)

get_target_property(LIBS_LIST licanc LINK_LIBRARIES)
message(STATUS "Libraries linked to licanc: ${LIBS_LIST}")