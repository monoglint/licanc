add_library(package)

target_sources(package
    PUBLIC
        FILE_SET CXX_MODULES FILES
            package.ixx
    PRIVATE

)

target_link_libraries(package
    PRIVATE
        frontend
)