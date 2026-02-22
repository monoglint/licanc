#pragma once

#include <cstddef>

namespace frontend::manager {
    // index of t_compilation_file in t_compilation_unit::files
    using t_file_id = size_t;

    // index of std::string in t_compilation_unit::identifier_pool
    using t_identifier_id = size_t;

    // index of std::string in t_compilation_unit::string_literal_pool
    using t_string_literal_id = size_t;
};