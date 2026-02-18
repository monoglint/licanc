/*

primary interface header

*/

#pragma once

#include <string>

namespace licanc {
    struct t_lican_config {
        // directory for the entire lican project
        std::string project_path = "";
        std::string start_subpath = "";
        std::string app_name = "untitled_lican_app";
    };

    void compile(t_lican_config config);
}