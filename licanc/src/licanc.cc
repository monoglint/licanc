#include "licanc.hh"

#include <iostream>
#include <filesystem>

#include "frontend/manager.hh"
#include "util/panic.hh"

void licanc::assert_licanc_config_validity(licanc::t_licanc_config& config) {
    config.project_path
}

licanc::t_compile_result licanc::compile(licanc::t_licanc_config config) {
    try {
        frontend::manager::t_compilation_unit unit(config);
        unit.compile();

        return unit.files.has_errors() ? t_compile_result::ERROR : t_compile_result::SUCCESS;
    }
    catch (util::t_panic_assertion assertion) {
        std::cerr << "Licanc failed and had to terminate:\n";
        std::cerr << assertion.what();
    }

    // return is here under the case of another type of exception being caught
    return t_compile_result::TERMINATED;
}