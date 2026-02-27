#pragma once

#include <cstdint>

#include "util/safe_id.hh"

/*

NOTE: ALL OF THESE ID TYPES ARE std::size_t BECAUSE std::size_t IS THE TYPE USED FOR INDEXES IN MOST C++ CONTAINERS.
DO NOT DOWNCAST THESE TYPES, THEY SHOULD REMAIN std::size_t

*/


namespace frontend::manager {
    // index of t_compilation_file in t_compilation_unit::files
    using t_file_id = util::t_safe_id<struct t_file_id_tag>; 

    // index of std::string in t_compilation_unit::identifier_pool
    using t_identifier_id = util::t_safe_id<struct t_identifier_id_tag>;

    // index of std::string in t_compilation_unit::string_literal_pool
    using t_string_literal_id = util::t_safe_id<struct t_string_literal_id_tag>;

    using t_type_name_id = util::t_safe_id<struct t_type_name_id_tag>;

    using t_constexpr_id = util::t_safe_id<struct t_constexpr_id_tag>;
};