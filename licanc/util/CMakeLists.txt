add_library(util)

target_sources(util
    PUBLIC
        FILE_SET CXX_MODULES FILES
            util.ixx
            ansi_format.ixx
            arena.ixx
            bitflags.ixx
            hash.ixx
            intern_pool.ixx
            logger.ixx
            panic.ixx
            ptr.ixx
            safe_id.ixx
            span.ixx
            vector_hasher.ixx
)