#include "frontend/scan/parser.hh"

#include <filesystem>

namespace frontend::scan::parser {
    namespace {
        // Takes the path the programmer described in an import item and converts it to a standardized path that should always be used. 
        
        /*
        
        ways a path can be formatted:
        
        1. with no cd
        import "C:/u"
        
        
        */
        [[nodiscard]]
        std::string standardize_file_path(t_parser_context& context, std::string path) {
            
        }
    }
}


void frontend::scan::parser::parse(t_parser_context context) {

}

void frontend::scan::parser::build_test_ast(t_parser_context context) {

}