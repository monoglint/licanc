add_subdirectory(driver)
add_subdirectory(frontend)
add_subdirectory(driver_base)
add_subdirectory(util)

add_library(licanc_library INTERFACE)

target_link_libraries(licanc_library INTERFACE
    driver
    frontend
    driver_base
    util
)

add_executable(licanc main.cxx)

target_link_libraries(licanc PRIVATE licanc_library)

get_target_property(LIBS_LIST licanc LINK_LIBRARIES)
message(STATUS "Libraries linked to licanc: ${LIBS_LIST}")