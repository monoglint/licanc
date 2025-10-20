#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "core.hh"
#include "arena.hh"
#include "util.hh"
#include "ast.hh"
#include "semantic.hh"

namespace core {
    namespace sym {
        enum class symbol_type : uint8_t {
            ROOT,

            SPEC_FUNCTION,
            SPEC_STRUCT,
            SPEC_TYPEDEC,
            SPEC_PRIMITIVE,

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

        using t_specification_map = std::unordered_map<const t_symbol_list, const t_symbol_id, liutil::vector_hasher<t_symbol_id>>;
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
            
            std::reference_wrapper<const ast::t_node_list> template_parameter_list; // expr_identifier
            // Does not need to be initialized in constructor
            t_specification_map specification_map;
        };

        struct specification : symbol {
            specification(const symbol_type type, t_symbol_list&& template_argument_list, const t_symbol_id declaration_id)
                : symbol(type), template_argument_list(std::move(template_argument_list)), declaration_id(declaration_id) {}

            specification(const symbol_type type, const t_symbol_id declaration_id)
                : symbol(type), declaration_id(declaration_id) {}

            virtual ~specification() = default;

            t_symbol_list template_argument_list = {}; // type_wrapper
            t_symbol_id declaration_id; // decl_function
        };

        // Example use would be the result of failing to resolve a symbol. "Variable 'a' has not been declared in the current socpe." 
        struct sym_invalid : symbol {
            sym_invalid()
                : symbol(symbol_type::INVALID) {}
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

        struct spec_function : specification {
            spec_function(t_symbol_list&& type_argument_list, const t_symbol_id declaration_id)
                : specification(symbol_type::SPEC_FUNCTION, std::move(type_argument_list), declaration_id) {}

            t_symbol_id return_type_id = INVALID_SYMBOL_ID; // type_wrapper
        };

        struct decl_function : specifiable {
            decl_function(const ast::expr_function& node)
                : specifiable(symbol_type::DECL_FUNCTION, node.template_parameter_list), node(node) {}

            decl_function(const ast::expr_function& node, const t_symbol_id _return_type_id)
                : decl_function(node) {
                    return_type_id = _return_type_id;
                }

            std::reference_wrapper<const ast::expr_function> node;
            
            // More of a temporary value for prescan runs. Will be re-processed during specification.
            t_symbol_id return_type_id = INVALID_SYMBOL_ID; // Can be unspecified

            // | Nothing appended to these during instantiation. |
            // V                                                 V

            t_symbol_list overloads;
        };

        struct spec_struct : specification {
            spec_struct(const t_symbol_id declaration_id)
                : specification(symbol_type::SPEC_STRUCT, declaration_id) {}
        };

        struct decl_struct : specifiable {
            decl_struct(const ast::item_struct_declaration& node)
                : specifiable(symbol_type::DECL_STRUCT, node.template_parameter_list), node(node) {}

            std::reference_wrapper<const ast::item_struct_declaration> node;
        };

        // NOTE: Primitives are not designed to work with templates. Specifications are just here for general architecture uniformity.
        struct spec_primitive : specification {
            spec_primitive(const t_symbol_id declaration_id)
                : specification(symbol_type::SPEC_PRIMITIVE, declaration_id) {}
        };

        // NOTE: Primitives are not designed to work with templates. Specifications are just here for general architecture uniformity.
        struct decl_primitive : specifiable {
            decl_primitive(const size_t size, const size_t alignment)
                : specifiable(symbol_type::DECL_PRIMITIVE, _empty_template_parameter_list), size(size), alignment(alignment) {}

            size_t size = 0;
            size_t alignment = 0;

            // fake ast node list to satisfy specification
            ast::t_node_list _empty_template_parameter_list = {};
        };
        
        struct type_wrapper : symbol {
            type_wrapper(const t_symbol_id wrapee_id, const core::e_type_qualifier qualifier = core::e_type_qualifier::NONE)
                : symbol(symbol_type::TYPE_WRAPPER), wrapee_id(wrapee_id), qualifier(qualifier) {}

            // point to INVALID_SYMBOL_ID for unspecified
            t_symbol_id wrapee_id; // specification | type_wrapper
            core::e_type_qualifier qualifier; // if wrapee is another wrapper, then this can be something other than NONE
        };

        struct sym_root : symbol {
            sym_root()
                : symbol(symbol_type::ROOT) {}

            t_symbol_id global_module = INVALID_SYMBOL_ID;
        };  

        struct arena_symbol {
            template <typename T>
            arena_symbol(T&& sym)
                : _raw(std::forward<T>(sym)) {}

            std::variant <
                sym_invalid,
                decl_variable,
                decl_module,

                spec_function,
                decl_function,

                spec_struct,
                decl_struct,

                spec_primitive,
                decl_primitive,

                type_wrapper,

                sym_root
            > _raw;
        };

        struct symbol_arena : liutil::arena<t_symbol_id, symbol, arena_symbol> {
            std::string pretty_debug(const core::liprocess& process, const core::ast::ast_arena& ast_arena, const t_symbol_id id);

            // Contains names for declarations and other symbols. Used purely for debugging.
            std::unordered_map<t_symbol_id, core::t_identifier_id> symbol_name_map;

            inline t_symbol_id unwrap_type_wrapper(const type_wrapper& type) const {
                if (type.wrapee_id != INVALID_SYMBOL_ID && get_base_ptr(type.wrapee_id)->type == symbol_type::TYPE_WRAPPER)
                    return unwrap_type_wrapper(get_as<type_wrapper>(type.wrapee_id));
                return type.wrapee_id;
            }

            inline std::string get_symbol_name(const core::liprocess& process, const t_symbol_id symbol_id) const {
                const auto iterator = symbol_name_map.find(symbol_id);

                if (iterator == symbol_name_map.end())
                    return "<unnamed>";
                else
                    return process.identifier_lookup.get(iterator->second);
            }
        };
    }
}