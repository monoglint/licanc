#include "frontend/sema/symbol_registrar.hh"

#include <iostream>

#include "frontend/sema/sema.hh"

// utility
namespace {
    using namespace frontend;

    struct t_unit_file_pair {
        manager::t_compilation_unit& unit;
        manager::t_compilation_file& file;
        manager::t_file_id file_id;
    };

    bool check_identifier_declarability(sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node) {
        return module_declaration_sym.declarations.find(identifier_node.identifier_id) == module_declaration_sym.declarations.end();
    }

    bool assert_identifier_declarability(t_unit_file_pair& ufp, sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node) {
        if (!check_identifier_declarability(module_declaration_sym, identifier_node)) {
            std::string identifier = ufp.unit.compile_time_data.identifier_pool.get(identifier_node.identifier_id).value().get();
            ufp.unit.logger.add_error(ufp.file_id, identifier_node.span, "Multiple declarations of " + identifier);
            return false;
        }

        return true;
    }

    void declare_symbol(t_unit_file_pair& ufp, sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node, sema::sym::t_sym_id declaration_sym_id) {
        if (!assert_identifier_declarability(ufp, module_declaration_sym, identifier_node))
            return;

        module_declaration_sym.declarations.insert({identifier_node.identifier_id, declaration_sym_id});
    }
}

// walkers
namespace {
    using namespace frontend;

    void walk(t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym, scan::ast::t_global_declaration_item& declaration_node) {
        // note a symbol will always be made no matter the conditions. it will only fail to be appended if its declarability is nulled
        
        sema::sym::t_sym_id declaration_sym_id = ufp.file.symbol_table.emplace<sema::sym::t_global_declaration>(sema::sym::t_sym_id::INVALID);
        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // bypass because main walker function verified existence

        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

    void walk(t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym, scan::ast::t_function_declaration_item& declaration_node) {
        sema::sym::t_sym_id template_sym_id = ufp.file.symbol_table.emplace<sema::sym::t_function_template>();
        sema::sym::t_sym_id declaration_sym_id = ufp.file.symbol_table.emplace<sema::sym::t_function_declaration>(template_sym_id);
        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get();
        
        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

// macro will be gone by production
#define TEST_TYPE(T) if (ufp.file.ast.is<T>(node_id)) walk(ufp, declaration_sym, ufp.file.ast.get<T>(node_id).value().get());

    void walk(t_unit_file_pair& ufp, scan::ast::t_module_declaration_item& declaration_node) {
        sema::sym::t_module_declaration& declaration_sym = ufp.file.symbol_table.emplace_get<sema::sym::t_module_declaration>();

        for (scan::ast::t_node_id node_id : declaration_node.items) {
            TEST_TYPE(scan::ast::t_global_declaration_item)
            else TEST_TYPE(scan::ast::t_function_declaration_item)
        }
    }

#undef TEST_TYPE

    void walk(t_unit_file_pair& ufp, scan::ast::t_root& node) {
        ufp.file.symbol_table.emplace<sema::sym::t_root>(1); // points to the module that will be made in the next statement

        walk(ufp, ufp.file.ast.get<scan::ast::t_module_declaration_item>(node.global_module).value().get());
    }
}

void frontend::sema::symbol_registrar::register_symbols(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    manager::t_compilation_unit::t_get_file_result get_file_result = unit.get_file(file_id); // confirmed by semantic_analyzer.cc
    manager::t_compilation_file& file = get_file_result.value();
    
    if (!file.ast.is<scan::ast::t_root>(0)) {
        unit.logger.add_internal_error(file_id, util::t_span(), "Symbol registrar discovered a malformed AST - root node is not in the right place.");
        return;
    }
    
    t_unit_file_pair ufp(unit, file, file_id);

    walk(ufp, file.ast.get<scan::ast::t_root>(0).value().get());
}