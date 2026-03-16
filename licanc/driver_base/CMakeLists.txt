add_library(driver_base)

target_sources(driver_base
    PUBLIC
        FILE_SET CXX_MODULES FILES
            driver_base.ccm
    PRIVATE
)

target_link_libraries(driver_base
    PRIVATE util
)