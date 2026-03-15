module driver;

import <filesystem>;
import <iostream>

import util;
import manager;

void licanc::assert_licanc_config_validity(licanc::LicancConfig& config) {
    util::panic_assert(std::filesystem::exists(config.project_path), "The main project path does not exist.");
    util::panic_assert(std::filesystem::exists(config.start_path), "The entry point file path does not exist.");
}

void licanc::correct_licanc_config(licanc::LicancConfig& config) {
    config.project_path = std::filesystem::absolute(config.project_path).string();
    config.start_path = std::filesystem::absolute(config.start_path).string();
}

licanc::CompileResult licanc::compile(licanc::LicancConfig config) {
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