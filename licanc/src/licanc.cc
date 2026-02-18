#include "licanc.hh"

#include <iostream>

#include "frontend/manager.hh"

void licanc::compile(licanc::t_lican_config config) {
    frontend::manager::t_frontend_config frontend_config;

    std::cout << "Beginning compilation for """ << config.app_name << """\n";

    frontend_config.project_path = config.project_path;
    frontend_config.start_subpath = config.start_subpath;

    frontend::manager::t_compilation_unit unit(frontend_config);


}