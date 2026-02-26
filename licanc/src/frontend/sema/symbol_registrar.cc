/*

THE SYMBOL REGISTRAR SHOULD ONLY CREATE DECLARATIONS WITH NO TYPE INFO OR ANYTHING EXCEPT CONNECTIONS TO THE AST IF NECESSARY.
ALL SYMBOL REFERENCES NEED TO HOLD FOR A LATER PASS OF THE AST

*/

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
            std::string identifier = ufp.unit.compile_time_data.identifier_pool.get(identifier_node.identifier_id).value().get(); // explicit bypass
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

    void walk(scan::ast::t_global_declaration_item& declaration_node, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        dec x: T = 5
        dec x
        
        */
        sema::sym::t_sym_id declaration_sym_id = ufp.unit.symbol_table.emplace<sema::sym::t_global_declaration>();
        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // bypass because main walker function verified existence

        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

    void walk(scan::ast::t_function_declaration_item& declaration_node, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        dec add<T>(a: T, b: T): T
        dec add

        only initialize basic identifiers and ast connections for the declaration

        */
        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass
        scan::ast::t_function_template& template_node = ufp.file.ast.get<scan::ast::t_function_template>(declaration_node.function_template).value().get(); // explicit bypass
        
        sema::sym::t_sym_id base_function_sym_id = ufp.unit.symbol_table.push<sema::sym::t_function>({.syntactic_function = template_node.base});

        sema::sym::t_sym_id template_sym_id = ufp.unit.symbol_table.push<sema::sym::t_function_template>({.base = base_function_sym_id});
        sema::sym::t_sym_id declaration_sym_id = ufp.unit.symbol_table.push<sema::sym::t_function_declaration>({.function_template = template_sym_id});
        
        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

    void walk(scan::ast::t_struct_declaration_item& declaration_node, t_unit_file_pair&ufp, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        struct vector2<T>{ dec x: T = 0 dec y: T = 0 }
        struct vector2

        */

        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass
        scan::ast::t_struct_template& template_node = ufp.file.ast.get<scan::ast::t_struct_template>(declaration_node.struct_template).value().get(); // explicit bypass
        
        sema::sym::t_sym_id base_struct_sym_id = ufp.unit.symbol_table.push<sema::sym::t_struct>({.syntactic_struct = template_node.base});

        sema::sym::t_sym_id template_sym_id = ufp.unit.symbol_table.push<sema::sym::t_struct_template>({.base = base_struct_sym_id});
        sema::sym::t_sym_id declaration_sym_id = ufp.unit.symbol_table.push<sema::sym::t_struct_declaration>({.struct_template = template_sym_id});
        
        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);        
    }

    // forward declaration just for walk -> module_declaration_item
    void eval_declarations(scan::ast::t_node_ids& nodes, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym);

    void walk(scan::ast::t_module_declaration_item& declaration_node, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        module my_module { items }
        module my_module { items }

        */
        scan::ast::t_identifier& declaration_name_node = ufp.file.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass

        sema::sym::t_sym_id declaration_sym_id = ufp.unit.symbol_table.emplace<sema::sym::t_module_declaration>();
        sema::sym::t_module_declaration& declaration_sym = ufp.unit.symbol_table.get<sema::sym::t_module_declaration>(declaration_sym_id).value().get(); // explicit bypass
        
        declare_symbol(ufp, parent_module_sym, declaration_name_node, declaration_sym_id);
        eval_declarations(declaration_node.items, ufp, declaration_sym);
    }

    void walk(scan::ast::t_import_item& node, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym) {
        // by lican rules, an imported file's module should be fully finished before the analyzer even hits the current file
        sema::sym::t_sym_id import_marker_id = ufp.unit.symbol_table.push<sema::sym::t_import_marker>({.target_file_module = node.resolved_file_id});        

        /*

        ; file: main.li
        module outer {
            import "io"
            module inner {
                import "io"
                import "io" ; warning: duplicate import flags in module "inner"
            }
        }

        ; both are good
        outer..io..print()
        outer..inner..io..print()
        
        */

        parent_module_sym.import_markers.push_back(import_marker_id);
    }

    // this function is very boilerplate, we can fix that
    void eval_declarations(scan::ast::t_node_ids& nodes, t_unit_file_pair& ufp, sema::sym::t_module_declaration& parent_module_sym) {
        for (scan::ast::t_node_id node_id : nodes) {
            switch (ufp.file.ast.index_of(node_id).value()) { // bypass optional because we are trusting the parser
                case scan::ast::t_ast::get_index<scan::ast::t_global_declaration_item>():
                    walk(ufp.file.ast.get<scan::ast::t_global_declaration_item>(node_id).value().get(), ufp, parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_function_declaration_item>():
                    walk(ufp.file.ast.get<scan::ast::t_function_declaration_item>(node_id).value().get(), ufp, parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_struct_declaration_item>():
                    walk(ufp.file.ast.get<scan::ast::t_struct_declaration_item>(node_id).value().get(), ufp, parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_module_declaration_item>():
                    walk(ufp.file.ast.get<scan::ast::t_module_declaration_item>(node_id).value().get(), ufp, parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_import_item>():
                    walk(ufp.file.ast.get<scan::ast::t_import_item>(node_id).value().get(), ufp, parent_module_sym);
                    break;
                default:
                    // as dumb as this sounds, explicit bypass
                    ufp.unit.logger.add_internal_error(ufp.file_id, ufp.file.ast.get_base(node_id).value().get().span, "Reached unexpected node type."); 
                    break;
            }
        }
    }

    void walk(scan::ast::t_root& node, t_unit_file_pair& ufp) {
        // normally we do children first then base, but the root should be indexed at 0 for easier access than the last
        ufp.unit.symbol_table.emplace<sema::sym::t_root>(sema::sym::t_sym_id{1});
        sema::sym::t_module_declaration& declaration_sym = ufp.unit.symbol_table.emplace_get<sema::sym::t_module_declaration>(); // unnammed module that represents the top

        scan::ast::t_module_declaration_item& declaration_node = ufp.file.ast.get<scan::ast::t_module_declaration_item>(node.global_module).value().get(); // explicit bypass
        eval_declarations(declaration_node.items, ufp, declaration_sym);
    }
}

void frontend::sema::symbol_registrar::register_symbols(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    manager::t_compilation_unit::t_get_file_result get_file_result = unit.get_file(file_id); // confirmed by semantic_analyzer.cc
    manager::t_compilation_file& file = get_file_result.value();
    
    if (!file.ast.is<scan::ast::t_root>(scan::ast::t_node_id{0})) {
        unit.logger.add_internal_error(file_id, util::t_span(), "Symbol registrar discovered a malformed AST - root node is not in the right place.");
        return;
    }
    
    t_unit_file_pair ufp(unit, file, file_id);

    walk(file.ast.get<scan::ast::t_root>(scan::ast::t_node_id{0}).value().get(), ufp);
}