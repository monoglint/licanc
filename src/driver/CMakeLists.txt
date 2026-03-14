add_library(driver
    licanc.hh
    src/licanc.cc
)

target_include_directories(driver PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
