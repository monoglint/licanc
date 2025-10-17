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

            TYPE_WRAPPER,
        };

        using t_symbol_id = size_t;
        using t_symbol_list = std::vector<t_symbol_id>;

        using t_specification_map = std::unordered_map<t_symbol_list, t_symbol_id, liutil::vector_hasher<t_symbol_id>>;
        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            virtual ~symbol() = default;
            symbol_type type;
        };

        struct specifiable : symbol {
            specifiable(const symbol_type type, const ast::t_node_list& template_parameter_list)
                : symbol(type), template_parameter_list(template_parameter_list) {}
            
            virtual ~specifiable() = default;
            
            ast::t_node_list template_parameter_list; // expr_identifier
            // Does not need to be initialized in constructor
            t_specification_map specification_map;
        };

        struct specification : symbol {
            specification(const symbol_type type, const t_symbol_list& template_argument_list, const t_symbol_id declaration_id)
                : symbol(type), template_argument_list(template_argument_list), declaration_id(declaration_id) {}

            specification(const symbol_type type)
                : symbol(type) {}

            virtual ~specification() = default;

            t_symbol_list template_argument_list; // type_wrapper
            t_symbol_id declaration_id; // decl_function
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

        struct decl_module : symbol { 
            decl_module()
                : symbol(symbol_type::DECL_MODULE) {}

            std::unordered_map<t_identifier_id, t_symbol_id> declaration_map; // decl

            // Quick lookup
            inline bool has_item(const t_identifier_id identifier) const {
                return declaration_map.find(identifier) != declaration_map.end();
            }
        };

        struct info_function_specification : specification {
            info_function_specification(const t_symbol_id return_type, const t_symbol_list& type_argument_list, const t_symbol_id declaration_id)
                : specification(symbol_type::INFO_FUNCTION_SPECIFICATION, type_argument_list, declaration_id), return_type(return_type) {}

            t_symbol_id return_type; // type_wrapper
        };

        struct decl_function : specifiable {
            decl_function(const ast::expr_function& node)
                : specifiable(symbol_type::DECL_FUNCTION, node.template_parameter_list), node(node) {}

            const ast::expr_function& node;
            
            // | Nothing appended to these during instantiation. |
            // V                                                 V

            t_symbol_list overloads;
        };

        struct info_struct_specification : specification {
            info_struct_specification()
                : specification(symbol_type::INFO_STRUCT_SPECIFICATION) {}
        };

        struct decl_struct {};
        
        struct type_wrapper : symbol {
            type_wrapper(const t_symbol_id wrapee_id, const core::e_type_qualifier qualifier)
                : symbol(symbol_type::TYPE_WRAPPER), wrapee_id(wrapee_id), qualifier(qualifier) {}

            // point to INVALID_SYMBOL_ID for unspecified
            t_symbol_id wrapee_id; // specification | type_wrapper
            core::e_type_qualifier qualifier; // if wrapee is another wrapper, then this can be something other than NONE
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

                info_function_specification,
                decl_function,

                info_struct_specification,
                decl_struct,

                type_wrapper,

                sym_root
            > _raw;
        };

        struct symbol_arena : liutil::arena<t_symbol_id, symbol, arena_symbol> {

        };
    }
}