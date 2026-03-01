/*

primary interface header

*/

#pragma once

#include <vector>
#include <string>

namespace licanc {
    // all contents of config is checked in validate_licanc_config
    struct t_licanc_config {
        std::string project_path;
        std::string start_path;
        std::string app_name = "untitled_lican_app";

        std::vector<std::string> target_import_paths;
    };

    enum class t_compile_result {
        SUCCESS,
        ERROR,
        TERMINATED,
    };

    void assert_licanc_config_validity(t_licanc_config& config);
    void correct_licanc_config(licanc::t_licanc_config& config);

    t_compile_result compile(t_licanc_config config);
}