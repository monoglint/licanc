#include "symbol.hh"
#include "ast.hh"
#include <unordered_set>
#include <functional>
#include <any>

// Deep recursive pretty-printer for symbols. Follows symbol references and prints referenced AST nodes
// when an ast_arena is available in the provided process.
void core::sym::symbol_arena::pretty_debug(const core::liprocess& process, const t_symbol_id id, std::string& buffer, uint8_t indent) {
    std::unordered_set<t_symbol_id> visited;

    // try to find the first ast_arena in the process's file list (if any)
    const core::ast::ast_arena* ast = nullptr;
    for (const auto& f : process.file_list) {
        if (!f.dump_ast_arena.has_value()) continue;
        try {
            ast = &std::any_cast<const core::ast::ast_arena&>(f.dump_ast_arena);
            break;
        } catch (const std::bad_any_cast&) {
            continue;
        }
    }

    std::function<void(const t_symbol_id, uint8_t)> impl;
    impl = [&](const t_symbol_id sid, uint8_t ind) {
        if (visited.find(sid) != visited.end()) {
            buffer += liutil::indent_repeat(ind);
            buffer += "<symbol ";
            buffer += std::to_string(sid);
            buffer += " (already printed)>\n";
            return;
        }

        visited.insert(sid);

        const arena_symbol& an = list[sid];
        const symbol* base = get_base_ptr(sid);
        if (!base) {
            buffer += liutil::indent_repeat(ind) + "<null symbol>\n";
            return;
        }

        switch (base->type) {
            case symbol_type::ROOT: {
                const auto& v = std::get<sym_root>(an._raw);
                buffer += liutil::indent_repeat(ind);
                buffer += "sym_root (";
                buffer += std::to_string(sid);
                buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1);
                buffer += "global_module: "; buffer += std::to_string(v.global_module); buffer += "\n";
                impl(v.global_module, ind+2);
                break;
            }

            case symbol_type::INFO_FUNCTION_SPECIFICATION: {
                const auto& v = std::get<info_function_specification>(an._raw);
                buffer += liutil::indent_repeat(ind);
                buffer += "info_function_specification ("; buffer += std::to_string(sid); buffer += ")\n";

                buffer += liutil::indent_repeat(ind+1); buffer += "return_type_id: "; buffer += std::to_string(v.return_type_id); buffer += "\n";
                impl(v.return_type_id, ind+2);

                buffer += liutil::indent_repeat(ind+1); buffer += "template_arguments:\n";
                for (const t_symbol_id& s : v.template_argument_list) impl(s, ind+2);

                buffer += liutil::indent_repeat(ind+1); buffer += "declaration_id: "; buffer += std::to_string(v.declaration_id); buffer += "\n";
                impl(v.declaration_id, ind+2);
                break;
            }

            case symbol_type::INFO_STRUCT_SPECIFICATION: {
                const auto& v = std::get<info_struct_specification>(an._raw);
                buffer += liutil::indent_repeat(ind);
                buffer += "info_struct_specification ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "template_arguments:\n";
                for (const t_symbol_id& s : v.template_argument_list) impl(s, ind+2);
                buffer += liutil::indent_repeat(ind+1); buffer += "declaration_id: "; buffer += std::to_string(v.declaration_id); buffer += "\n";
                impl(v.declaration_id, ind+2);
                break;
            }

            case symbol_type::INFO_TYPEDEC_SPECIFICATION:
                buffer += liutil::indent_repeat(ind); buffer += "info_typedec_specification (not implemented)\n";
                break;

            case symbol_type::INFO_PRIMITIVE_SPECIFICATION:
                buffer += liutil::indent_repeat(ind); buffer += "info_primitive_specification\n";
                break;

            case symbol_type::INVALID:
                buffer += liutil::indent_repeat(ind); buffer += "sym_invalid\n";
                break;

            case symbol_type::DECL_PRIMITIVE: {
                const auto& v = std::get<decl_primitive>(an._raw);
                buffer += liutil::indent_repeat(ind); buffer += "decl_primitive ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "size: "; buffer += std::to_string(v.size); buffer += "\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "alignment: "; buffer += std::to_string(v.alignment); buffer += "\n";
                break;
            }

            case symbol_type::DECL_VARIABLE: {
                const auto& v = std::get<decl_variable>(an._raw);
                buffer += liutil::indent_repeat(ind); buffer += "decl_variable ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "value_type_node_id: "; buffer += std::to_string(v.value_type); buffer += "\n";
                if (ast) ast->pretty_debug(process, v.value_type, buffer, ind+2);
                break;
            }

            case symbol_type::DECL_FUNCTION: {
                const auto& v = std::get<decl_function>(an._raw);
                buffer += liutil::indent_repeat(ind); buffer += "decl_function ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "return_type_id: "; buffer += std::to_string(v.return_type_id); buffer += "\n";
                impl(v.return_type_id, ind+2);

                buffer += liutil::indent_repeat(ind+1); buffer += "template_parameters (ast ids):\n";
                for (const auto& pid : v.template_parameter_list) {
                    if (ast) ast->pretty_debug(process, pid, buffer, ind+2);
                    else { buffer += liutil::indent_repeat(ind+2); buffer += std::to_string(pid); buffer += "\n"; }
                }

                buffer += liutil::indent_repeat(ind+1); buffer += "overloads:\n";
                for (const auto& s : v.overloads) impl(s, ind+2);
                break;
            }

            case symbol_type::DECL_STRUCT:
                buffer += liutil::indent_repeat(ind); buffer += "decl_struct (not implemented)\n";
                break;

            case symbol_type::DECL_ENUM:
                buffer += liutil::indent_repeat(ind); buffer += "decl_enum (not implemented)\n";
                break;

            case symbol_type::DECL_MODULE: {
                const auto& v = std::get<decl_module>(an._raw);
                buffer += liutil::indent_repeat(ind); buffer += "decl_module ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "declarations:\n";
                for (const auto& kv : v.declaration_map) {
                    buffer += liutil::indent_repeat(ind+2); buffer += "id: "; buffer += std::to_string(kv.first); buffer += " => symbol: "; buffer += std::to_string(kv.second); buffer += "\n";
                    impl(kv.second, ind+3);
                }
                break;
            }

            case symbol_type::DECL_TYPEDEC:
                buffer += liutil::indent_repeat(ind); buffer += "decl_typedec (not implemented)\n";
                break;

            case symbol_type::TYPE_WRAPPER: {
                const auto& v = std::get<type_wrapper>(an._raw);
                buffer += liutil::indent_repeat(ind); buffer += "type_wrapper ("; buffer += std::to_string(sid); buffer += ")\n";
                buffer += liutil::indent_repeat(ind+1); buffer += "wrapee_id: "; buffer += std::to_string(v.wrapee_id); buffer += "\n";
                impl(v.wrapee_id, ind+2);
                buffer += liutil::indent_repeat(ind+1); buffer += "qualifier: "; buffer += std::to_string(static_cast<int>(v.qualifier)); buffer += "\n";
                break;
            }

            default:
                buffer += liutil::indent_repeat(ind); buffer += "<unknown symbol>\n";
                break;
        }
    };

    impl(id, indent);
}
