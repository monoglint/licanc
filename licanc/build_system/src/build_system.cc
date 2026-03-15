module;

#include <filesystem>
#include <iostream>

module build_system;

import util;
import manager;

void build_system::assert_licanc_config_validity(build_system::LicancConfig& config) {
    util::panic_assert(std::filesystem::exists(config.project_path), "The main project path does not exist.");
    util::panic_assert(std::filesystem::exists(config.start_path), "The entry point file path does not exist.");
}

void build_system::correct_licanc_config(build_system::LicancConfig& config) {
    config.project_path = std::filesystem::absolute(config.project_path).string();
    config.start_path = std::filesystem::absolute(config.start_path).string();
}

build_system::CompileResult build_system::compile(build_system::LicancConfig config) {
    try {
        assert_licanc_config_validity(config);
        correct_licanc_config(config);

        // LATER: compare hashed config with cached compilations and load the frontend unit on those
        

        manager::EngineContext engine_context(config.project_path, config.start_path);
        manager::CompilationEngine engine(engine_context);

        engine.compile();

        return engine.file_manager.has_errors() ? CompileResult::ERROR : CompileResult::SUCCESS;
    }
    catch (util::PanicAssertion assertion) {
        std::cerr << "Licanc failed and had to terminate:\n";
        std::cerr << assertion.what();
    }

    // return is here under the case of another type of exception being caught
    return CompileResult::TERMINATED;
}