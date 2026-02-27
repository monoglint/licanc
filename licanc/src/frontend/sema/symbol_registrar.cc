/*

THE SYMBOL REGISTRAR SHOULD ONLY CREATE DECLARATIONS WITH NO TYPE INFO OR ANYTHING EXCEPT CONNECTIONS TO THE AST IF NECESSARY.
ALL SYMBOL REFERENCES NEED TO HOLD FOR A LATER PASS OF THE AST

*/

#include "frontend/sema/symbol_registrar.hh"

#include <iostream>

#include "frontend/sema/sema.hh"

namespace {
    using frontend::sema::symbol_registrar::t_registrar_context;
    namespace scan = frontend::scan;
    namespace sema = frontend::sema;
    
    bool check_identifier_declarability(sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node) {
        return module_declaration_sym.declarations.find(identifier_node.identifier_id) == module_declaration_sym.declarations.end();
    }

    bool assert_identifier_declarability(sema::symbol_registrar::t_registrar_context& context, sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node) {
        if (!check_identifier_declarability(module_declaration_sym, identifier_node)) {
            std::string identifier = context.compile_time_data.identifier_pool.get(identifier_node.identifier_id).value().get(); // explicit bypass
            context.logger.add_error(context.file_id, identifier_node.span, "Multiple declarations of " + identifier);
            return false;
        }

        return true;
    }

    void declare_symbol(t_registrar_context& context, sema::sym::t_module_declaration& module_declaration_sym, scan::ast::t_identifier& identifier_node, sema::sym::t_sym_id declaration_sym_id) {
        if (!assert_identifier_declarability(context, module_declaration_sym, identifier_node))
            return;

        module_declaration_sym.declarations.insert({identifier_node.identifier_id, declaration_sym_id});
    }

    void walk(t_registrar_context& context, scan::ast::t_global_declaration_item& declaration_node, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        dec x: T = 5
        dec x
        
        */
        sema::sym::t_sym_id declaration_sym_id = context.symbol_table.push<sema::sym::t_global_declaration>({});
        scan::ast::t_identifier& declaration_name_node = context.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // bypass because main walker function verified existence

        declare_symbol(context, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

    void walk(t_registrar_context& context, scan::ast::t_function_declaration_item& declaration_node, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        dec add<T>(a: T, b: T): T
        dec add

        only initialize basic identifiers and ast connections for the declaration

        */
        scan::ast::t_identifier& declaration_name_node = context.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass
        scan::ast::t_function_template& template_node = context.ast.get<scan::ast::t_function_template>(declaration_node.function_template).value().get(); // explicit bypass
        
        sema::sym::t_sym_id base_function_sym_id = context.symbol_table.push<sema::sym::t_function>({.syntactic_function = template_node.base});

        sema::sym::t_sym_id template_sym_id = context.symbol_table.push<sema::sym::t_function_template>({.base = base_function_sym_id});
        sema::sym::t_sym_id declaration_sym_id = context.symbol_table.push<sema::sym::t_function_declaration>({.function_template = template_sym_id});
        
        declare_symbol(context, parent_module_sym, declaration_name_node, declaration_sym_id);
    }

    void walk(t_registrar_context& context, scan::ast::t_struct_declaration_item& declaration_node, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        struct vector2<T>{ dec x: T = 0 dec y: T = 0 }
        struct vector2

        */

        scan::ast::t_identifier& declaration_name_node = context.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass
        scan::ast::t_struct_template& template_node = context.ast.get<scan::ast::t_struct_template>(declaration_node.struct_template).value().get(); // explicit bypass
        
        sema::sym::t_sym_id base_struct_sym_id = context.symbol_table.push<sema::sym::t_struct>({
            .syntactic_struct = sema::sym::t_ast_reference{.node_id = template_node.base}
        });

        sema::sym::t_sym_id template_sym_id = context.symbol_table.push<sema::sym::t_struct_template>({.base = base_struct_sym_id});
        sema::sym::t_sym_id declaration_sym_id = context.symbol_table.push<sema::sym::t_struct_declaration>({.struct_template = template_sym_id});
        
        declare_symbol(context, parent_module_sym, declaration_name_node, declaration_sym_id);        
    }

    // forward declaration just for walk -> module_declaration_item
    void eval_declarations(t_registrar_context& context, scan::ast::t_node_ids& nodes, sema::sym::t_module_declaration& parent_module_sym);

    void walk(t_registrar_context& context, scan::ast::t_module_declaration_item& declaration_node, sema::sym::t_module_declaration& parent_module_sym) {
        /*
        
        module my_module { items }
        module my_module { items }

        */
        scan::ast::t_identifier& declaration_name_node = context.ast.get<scan::ast::t_identifier>(declaration_node.name).value().get(); // explicit bypass

        sema::sym::t_sym_id declaration_sym_id = context.symbol_table.push<sema::sym::t_module_declaration>({});
        sema::sym::t_module_declaration& declaration_sym = context.symbol_table.get<sema::sym::t_module_declaration>(declaration_sym_id).value().get(); // explicit bypass
        
        declare_symbol(context, parent_module_sym, declaration_name_node, declaration_sym_id);
        eval_declarations(context, declaration_node.items, declaration_sym);
    }

    void walk(t_registrar_context& context, scan::ast::t_import_item& node, sema::sym::t_module_declaration& parent_module_sym) {
        // by lican rules, an imported file's module should be fully finished before the analyzer even hits the current file
        // sema::sym::t_sym_id import_marker_id = context.symbol_table.push<sema::sym::t_import_marker>({.target_file_module = node.resolved_file_id});        

        // ^^^^^^^^^^ RIGHT NOW, NODE.RESOLVED_FILE_ID IS A FILE_ID, WE NEED A SYMBOL VERSION OF THAT

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

        // parent_module_sym.import_markers.push_back(import_marker_id);
    }

    // this function is very boilerplate, we can fix that
    void eval_declarations(t_registrar_context& context, scan::ast::t_node_ids& nodes, sema::sym::t_module_declaration& parent_module_sym) {
        for (scan::ast::t_node_id node_id : nodes) {
            switch (context.ast.index_of(node_id).value()) { // bypass optional because we are trusting the parser
                case scan::ast::t_ast::get_index<scan::ast::t_global_declaration_item>():
                    walk(context, context.ast.get<scan::ast::t_global_declaration_item>(node_id).value().get(), parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_function_declaration_item>():
                    walk(context, context.ast.get<scan::ast::t_function_declaration_item>(node_id).value().get(), parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_struct_declaration_item>():
                    walk(context, context.ast.get<scan::ast::t_struct_declaration_item>(node_id).value().get(), parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_module_declaration_item>():
                    walk(context, context.ast.get<scan::ast::t_module_declaration_item>(node_id).value().get(), parent_module_sym);
                    break;
                case scan::ast::t_ast::get_index<scan::ast::t_import_item>():
                    walk(context, context.ast.get<scan::ast::t_import_item>(node_id).value().get(), parent_module_sym);
                    break;
                default:
                    // as dumb as this sounds, explicit bypass
                    context.logger.add_message(context.file_id, context.ast.get_base(node_id).value().get().span, "Reached unexpected node type."); 
                    break;
            }
        }
    }

    void walk(t_registrar_context& context, scan::ast::t_root& node) {
        sema::sym::t_sym_id file_module_declaration_sym_id = context.symbol_table.emplace<sema::sym::t_module_declaration>();
        sema::sym::t_module_declaration& file_module_declaration_sym = context.symbol_table.get<sema::sym::t_module_declaration>(file_module_declaration_sym_id).value().get();

        context.symbol_table.get<sema::sym::t_root>(context.symbol_table.get_root_id()).value().get().file_modules.push_back(file_module_declaration_sym_id);

        eval_declarations(context, node.items, file_module_declaration_sym);
    }
}

void frontend::sema::symbol_registrar::register_symbols(t_registrar_context context) {   
    walk(context, context.ast.get<scan::ast::t_root>(context.ast.get_root_id()).value().get());
}