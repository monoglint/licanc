add_library(driver)

target_sources(driver
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            driver.ccm
    PRIVATE
        src/driver.cc
)

target_link_libraries(driver
    PRIVATE util
)