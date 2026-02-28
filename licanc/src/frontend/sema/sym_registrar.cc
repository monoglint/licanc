/*

THE SYMBOL REGISTRAR SHOULD ONLY CREATE decls WITH NO TYPE INFO OR ANYTHING EXCEPT CONNECTIONS TO THE AST IF NECESSARY.
ALL SYMBOL REFERENCES NEED TO HOLD FOR A LATER PASS OF THE AST

*/

#include "frontend/sema/sym_registrar.hh"

#include <iostream>

#include "frontend/sema/sema.hh"

#include "util/panic.hh"

namespace {
    using frontend::sema::sym_registrar::t_registrar_context;
    namespace scan = frontend::scan;
    namespace sema = frontend::sema;
    
    bool check_identifier_declarability(sema::sym::t_module_decl* module_decl_sym, scan::ast::t_identifier* identifier_node) {
        return module_decl_sym->decls.find(identifier_node->identifier_id) == module_decl_sym->decls.end();
    }

    bool assert_identifier_declarability(sema::sym_registrar::t_registrar_context& context, sema::sym::t_module_decl* module_decl_sym, scan::ast::t_identifier* identifier_node) {
        if (!check_identifier_declarability(module_decl_sym, identifier_node)) {
            std::string identifier = context.compile_time_data.identifier_pool.get(identifier_node->identifier_id).value().get(); // explicit bypass
            context.logger.add_error(context.file_id, identifier_node->span, "Multiple decls of " + identifier);
            return false;
        }

        return true;
    }

    void declare_symbol(t_registrar_context& context, sema::sym::t_module_decl* module_decl_sym, scan::ast::t_identifier* identifier_node, sema::sym::t_decl* decl_sym) {
        if (!assert_identifier_declarability(context, module_decl_sym, identifier_node))
            return;

        module_decl_sym->decls.insert({identifier_node->identifier_id, decl_sym});
    }

    void walk(t_registrar_context& context, scan::ast::t_global_decl* decl_node, sema::sym::t_module_decl* parent_module_sym) {
        /*
        
        dec x: T = 5
        dec x
        
        */
        auto* decl_sym = context.sym_table.push<sema::sym::t_global_decl>(sema::sym::t_global_decl{});

        declare_symbol(context, parent_module_sym, decl_node->name, decl_sym);
    }

    void walk(t_registrar_context& context, scan::ast::t_function_decl* decl_node, sema::sym::t_module_decl* parent_module_sym) {
        /*
        
        dec add<T>(a: T, b: T): T
        dec add

        only initialize basic identifiers and ast connections for the decl

        */        
        auto* base_function_sym = context.sym_table.push<sema::sym::t_function>({
            .syntactic_function = decl_node->function_template->base
        });

        auto* template_sym = context.sym_table.push<sema::sym::t_function_template>({
            .base = base_function_sym
        });

        auto* decl_sym = context.sym_table.emplace<sema::sym::t_function_decl>(template_sym);
        
        declare_symbol(context, parent_module_sym, decl_node->name, decl_sym);
    }

    void walk(t_registrar_context& context, scan::ast::t_record_decl* decl_node, sema::sym::t_module_decl* parent_module_sym) {
        /*
        
        struct vector2<T>{ dec x: T = 0 dec y: T = 0 }
        struct vector2

        */

        auto* base_struct_sym = context.sym_table.push<sema::sym::t_record>(sema::sym::t_record{
            .syntactic_record = decl_node->record_template->base
        });

        auto* template_sym = context.sym_table.push<sema::sym::t_record_template>({
            .base = base_struct_sym
        });

        auto* decl_sym = context.sym_table.emplace<sema::sym::t_record_decl>(template_sym);
        
        declare_symbol(context, parent_module_sym, decl_node->name, decl_sym);        
    }

    // forward decl just for walk -> module_decl
    void eval_decls(t_registrar_context& context, scan::ast::t_decl_ptrs& nodes, sema::sym::t_module_decl* parent_module_sym);

    void walk(t_registrar_context& context, scan::ast::t_module_decl* decl_node, sema::sym::t_module_decl* parent_module_sym) {
        /*
        
        module my_module { decls }
        module my_module { decls }

        */

        auto* decl_sym = context.sym_table.emplace<sema::sym::t_module_decl>(parent_module_sym);
        
        declare_symbol(context, parent_module_sym, decl_node->name, decl_sym);
        eval_decls(context, decl_node->decls, decl_sym);
    }

    void walk(t_registrar_context& context, scan::ast::t_import_decl* node, sema::sym::t_module_decl* parent_module_sym) {
        // by lican rules, an imported file's module should be fully finished before the analyzer even hits the current file
        sema::sym::t_root* root_sym = context.sym_table.root_ptr;
        
        if (node->resolved_file_id.get() >= root_sym->file_modules.size())
            util::panic("The requested file (" + std::to_string(node->resolved_file_id.get()) + ") was not recognized by the semantic analyzer.");
        
        sema::sym::t_module_decl* target_file_module_sym = root_sym->file_modules[node->resolved_file_id.get()];

        auto* import_marker_sym = context.sym_table.push<sema::sym::t_import_marker>({
            .get_file_module = target_file_module_sym
        });        

        parent_module_sym->import_markers.push_back(import_marker_sym);

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
    }

    // this function is very boilerplate, we can fix that
    void eval_decls(t_registrar_context& context, scan::ast::t_decl_ptrs& nodes, sema::sym::t_module_decl* parent_module_sym) {
        for (scan::ast::t_decl* node : nodes) {
            switch (node->decl_type) {
                case scan::ast::t_decl_type::FUNCTION:
                    walk(context, static_cast<scan::ast::t_function_decl*>(node), parent_module_sym); 
                    break;
                case scan::ast::t_decl_type::GLOBAL:
                    walk(context, static_cast<scan::ast::t_global_decl*>(node), parent_module_sym);                 
                    break;
                case scan::ast::t_decl_type::IMPORT:
                    walk(context, static_cast<scan::ast::t_import_decl*>(node), parent_module_sym); 
                    break;
                case scan::ast::t_decl_type::MODULE:
                    walk(context, static_cast<scan::ast::t_module_decl*>(node), parent_module_sym); 
                    break;
                case scan::ast::t_decl_type::RECORD:
                    walk(context, static_cast<scan::ast::t_record_decl*>(node), parent_module_sym); 
                    break;
            }
        }
    }

    void walk(t_registrar_context& context, scan::ast::t_root* node) {
        auto* file_module_sym = context.sym_table.emplace<sema::sym::t_module_decl>();

        context.sym_table.root_ptr->file_modules.push_back(file_module_sym);

        eval_decls(context, node->decls, file_module_sym);
    }
}

void frontend::sema::sym_registrar::register_symbols(t_registrar_context context) {       
    walk(context, context.ast.root_ptr);
}