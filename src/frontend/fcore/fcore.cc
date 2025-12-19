#include "frontend/fcore/fcore.hh"

#include <fstream>

base::e_success fcore::s_compiler_data::add_file(const std::string& file_path) {
    std::ifstream file_stream(file_path);

    if (!file_stream.is_open())
        return base::e_success::FAILURE;

    file_name_map.emplace(file_path, file_process_list.size());
    file_process_list.emplace_back(file_path);

    return base::e_success::SUCCESS;
}