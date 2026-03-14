add_library(util
    ansi_format.hh
    arena.hh
    bitflags.hh
    hash.hh
    intern_pool.hh
    logger.hh
    panic.hh
    safe_id.hh
    span.hh
    vector_hasher.hh

    logger.cc
    panic.cc
    span.cc
)

target_include_directories(util PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)