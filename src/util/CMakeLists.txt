add_library(util)

target_sources(util
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            util.ccm
            ansi_format.ccm
            arena.ccm
            bitflags.ccm
            hash.ccm
            intern_pool.ccm
            logger.ccm
            panic.ccm
            safe_id.ccm
            span.ccm
            vector_hasher.ccm
)