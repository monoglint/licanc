/*

primary interface header

*/

#pragma once

#include <vector>
#include <string>

namespace licanc {
    // all contents of config is checked in validate_licanc_config
    struct t_licanc_config {
        const std::string project_path;
        const std::string start_subpath;
        const std::string app_name = "untitled_lican_app";

        const std::vector<std::string> target_import_paths
    };

    enum class t_compile_result {
        SUCCESS,
        ERROR,
        TERMINATED,
    };

    void assert_licanc_config_validity(t_licanc_config& config);
    t_compile_result compile(t_licanc_config config);
}