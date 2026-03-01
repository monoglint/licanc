#include "licanc.hh"

#include <iostream>
#include <filesystem>

#include "frontend/manager.hh"
#include "util/panic.hh"

void licanc::assert_licanc_config_validity(licanc::t_licanc_config& config) {
    util::panic_assert(std::filesystem::exists(config.project_path), "The main project path does not exist.");
    util::panic_assert(std::filesystem::exists(config.start_path), "The entry point file path does not exist.");
}

void licanc::correct_licanc_config(licanc::t_licanc_config& config) {
    config.project_path = std::filesystem::absolute(config.project_path).string();
    config.start_path = std::filesystem::absolute(config.start_path).string();
}

licanc::t_compile_result licanc::compile(licanc::t_licanc_config config) {
    try {
        assert_licanc_config_validity(config);
        correct_licanc_config(config);

        // LATER: compare hashed config with cached compilations and load the frontend unit on those
        frontend::manager::t_frontend_unit unit(config);
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