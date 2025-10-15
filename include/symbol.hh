#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include "core.hh"
#include "arena.hh"
#include "util.hh"

namespace core {
    namespace sym {
        enum class symbol_type : uint8_t {
            ROOT,

            INFO_FUNCTION_SPECIFICATION,
            INFO_STRUCT_SPECIFICATION,
            INFO_TYPEDEC_SPECIFICATION,

            INVALID,

            DECL_PRIMITIVE, // TYPE
            DECL_VARIABLE,
            DECL_FUNCTION, // TEMPLATABLE
            DECL_STRUCT, // TYPE - TEMPLATABLE
            DECL_ENUM, // TYPE
            DECL_MODULE,
            DECL_TYPEDEC, // TEMPLATABLE
        };

        using t_symbol_id = size_t;
        using t_symbol_list = std::vector<t_symbol_id>;

        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            virtual ~symbol() = default;
            symbol_type type;
        };

        // Example use would be the result of failing to resolve a symbol. "Variable 'a' has not been declared in the current socpe." 
        struct sym_invalid : symbol {
            sym_invalid()
                : symbol(symbol_type::INVALID) {}
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

        struct info_function_specification : symbol {
            t_symbol_id return_type; // any type decl symbol
            t_symbol_list type_argument_list; // any type decl symbol
            t_symbol_id declaration; // decl_function
        };

        struct decl_function : symbol {
            inline ast::t_node_list _make_parameter_type_list(const ast::ast_arena& arena, const ast::t_node_list& parameter_list) {
                ast::t_node_list _parameter_type_list = {};

                for (ast::t_node_id param_id : parameter_list) {
                    _parameter_type_list.push_back(arena.get_as<ast::expr_parameter>(param_id).value_type);
                }

                return _parameter_type_list;
            }

            // parameter_type_list isn't natively part of expr_function, so it will be generated during semantic analysis.
            // therefore, we can make it an rvalue reference to be moved in
            decl_function(const ast::t_node_id return_type, ast::t_node_list&& parameter_type_list, const ast::t_node_list& template_parameter_list)
                : symbol(symbol_type::DECL_FUNCTION), return_type(return_type), parameter_type_list(std::move(parameter_type_list)), template_parameter_list(template_parameter_list) {}

            decl_function(const ast::ast_arena& arena, const ast::expr_function& function)
                : decl_function(function.return_type, _make_parameter_type_list(arena, function.parameter_list), function.template_parameter_list) {}

            ast::t_node_id return_type; // expr_type
            ast::t_node_list parameter_type_list; // expr_type
            ast::t_node_list template_parameter_list; // expr_identifier
            
            // | Nothing appended to these during instantiation. |
            // V                                                 V

            t_symbol_list overloads;

            std::unordered_map<t_symbol_list, t_symbol_id, liutil::vector_hasher<t_symbol_id>> specification_map; // vector<any type node>, info_function_specification 
        };

        struct info_struct_specification {};
        struct decl_struct {};

        struct decl_module : symbol { 
            decl_module()
                : symbol(symbol_type::DECL_MODULE) {}

            std::unordered_map<t_identifier_id, t_symbol_id> declaration_map; // decl

            // Quick lookup
            inline bool has_item(const t_identifier_id identifier) const {
                return declaration_map.find(identifier) != declaration_map.end();
            }
        };

        struct sym_root : symbol {
            sym_root()
                : symbol(symbol_type::ROOT) {}

            t_symbol_id global_module;
        };

        struct sym_call_frame {
            // Remember; functions, enums, and structs are not meant to be declared on the local level ever.
            std::unordered_map<t_identifier_id, t_symbol_id> local_map; // decl_variable 
        };

        struct arena_symbol {
            template <typename T>
            arena_symbol(T&& node)
                : _raw(std::forward<T>(node)) {}

            std::variant <
                sym_invalid,
                decl_primitive,
                decl_variable,
                decl_module,

                sym_root
            > _raw;
        };

        struct symbol_arena : liutil::arena<t_symbol_id, symbol, arena_symbol> {

        };
    }
}

/* UNFINISHED - PROOF-OF-CONCEPT PROTOTYPE


    
*/