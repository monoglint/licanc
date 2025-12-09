#include "frontend/core/main.hh"

#include <fstream>

base::t_success fcore::t_compiler_data::add_file(const std::string& file_path) {
    std::ifstream file_stream(file_path);

    if (!file_stream.is_open())
        return base::FAILURE;

    file_name_map.emplace(file_path, file_list.size());
    file_list.emplace_back();

    return base::SUCCESS;
}