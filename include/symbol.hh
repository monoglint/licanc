#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include "core.hh"
#include "arena.hh"

namespace core {
    namespace sym {
        enum class symbol_type : uint8_t {
            ROOT,

            INFO_TEMPLATE_INSERTION,

            DECL_PRIMITIVE, // TYPE
            DECL_VARIABLE,
            DECL_FUNCTION,
            DECL_STRUCT, // TYPE
            DECL_ENUM,
            DECL_MODULE,
            DECL_TYPEDEC,
        };

        using t_symbol_id = size_t;
        using t_symbol_list = std::vector<t_symbol_id>;

        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            virtual ~symbol() = default;
            symbol_type type;
        };

        struct decl_primitive : symbol {
            decl_primitive(const size_t size, const size_t alignment)
                : symbol(symbol_type::DECL_PRIMITIVE), size(size), alignment(alignment) {}

            size_t size;
            size_t alignment;
        };
        
        struct decl_variable : symbol {
            decl_variable(const ast::t_node_id value_type)
                : symbol(symbol_type::DECL_VARIABLE), value_type(value_type) {}

            ast::t_node_id value_type; // expr_type
        };

        struct decl_function : symbol {
            inline ast::t_node_list _make_parameter_type_list(const ast::ast_arena& arena, const ast::t_node_list& parameter_list) {
                ast::t_node_list _paramter_type_list = {};

                for (ast::t_node_id param_id : parameter_list) {
                    _paramter_type_list.push_back(arena.get_as<ast::expr_parameter>(param_id).value_type);
                }

                return _paramter_type_list;
            }

            // parameter_type_list isn't natively part of expr_function, so it will be generated during semantic analysis.
            // therefore, we can make it an rvalue reference to be moved in
            decl_function(const ast::t_node_id return_type, ast::t_node_list&& parameter_type_list, ast::t_node_list template_list)
                : symbol(symbol_type::DECL_FUNCTION), return_type(return_type), parameter_type_list(std::move(parameter_type_list)), template_parameter_list(template_list) {}

            decl_function(const ast::ast_arena& arena, const ast::expr_function& function)
                : decl_function(function.return_type, _make_parameter_type_list(arena, function.parameter_list), template_parameter_list) {}

            ast::t_node_id return_type; // expr_type
            ast::t_node_list parameter_type_list; // expr_type
            ast::t_node_list &template_parameter_list; // expr_identifier
        };

        struct decl_module : symbol { 
            decl_module()
                : symbol(symbol_type::DECL_MODULE) {}

            std::unordered_map<t_identifier_id, t_symbol_id> declarations;
        };

        struct sym_root : symbol {
            sym_root()
                : symbol(symbol_type::ROOT) {}

            t_symbol_id global_module;
        };

        struct arena_symbol {
            template <typename T>
            arena_symbol(T&& node)
                : _raw(std::forward<T>(node)) {}

            std::variant <
                decl_function,
                decl_variable,
                decl_module,
                sym_root
            > _raw;
        };

        struct symbol_arena : liutil::arena<t_symbol_id, symbol, arena_symbol> {

        };
    }
}
