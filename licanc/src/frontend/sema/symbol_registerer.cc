#include "frontend/sema/symbol_registerer.hh"

#include <iostream>

#include "frontend/sema/sema.hh"

namespace {
    using namespace frontend;

    
    void walk(manager::t_compilation_file& file, scan::ast::t_global_declaration_item& node) {
        file.symbol_table.emplace<sema::sym::t_global_declaration>(0); // type will be filled in during a later step.
    }

    void walk(manager::t_compilation_file& file, scan::ast::t_function_declaration_item& node) {

    }

    template<typename T_NODE>
    void walk(manager::t_compilation_file& file, T_NODE& node) {
        if (std::same_as<T_NODE, scan::ast::t_global_declaration_item>) {
            
        }
        else if (std::same_as<T_NODE, scan::ast::t_function_declaration_item>) {

        }
    }

    void walk_root(manager::t_compilation_file& file, scan::ast::t_root& node) {
        for (scan::ast::t_node_id node_id : node.nodes) {
            if (file.ast.is<scan::ast::t_global_declaration_item>(node_id))
                walk(file, file.ast.get<scan::ast::t_global_declaration_item>(node_id));
            else if (file.ast.is<scan::ast::t_function_declaration_item>(node_id))
                walk(file, file.ast.get<scan::ast::t_function_declaration_item>(node_id));
        }
    }
}

void frontend::sema::symbol_registerer::register_symbols(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    std::cout << "registering symbols\n";
    
    manager::t_compilation_unit::t_get_file_result get_file_result = unit.get_file(file_id); // confirmed by semantic_analyzer.cc
    manager::t_compilation_file& file = get_file_result.value();
    
    if (!file.ast.is<scan::ast::t_root>(0)) {
        unit.logger.add_internal_error(file_id, util::t_span(), "Symbol registerer discovered a malformed AST - root node is not in the right place.");
        return;
    }
    
    walk_root(file, file.ast.get<scan::ast::t_root>(0));
}