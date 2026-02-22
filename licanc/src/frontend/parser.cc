#include "frontend/parser.hh"

void frontend::parser::parse(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id) {
    // note: parser needs to modify t_compilation_file's "import_node_ids" property with the node_id of import statements.
}

void frontend::parser::build_test_ast(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id) {
    
}