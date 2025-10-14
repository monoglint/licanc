/*

====================================================

Contains all of the AST node declarations. Used by the frontend and read by the backend.

====================================================

*/

#pragma once

#include <cstdint>
#include <memory>
#include <variant>

#include "util.hh"
#include "core.hh"
#include "token.hh"
#include "arena.hh"

namespace core {
    namespace ast {
        enum class node_type : uint8_t {
            ROOT,

            EXPR_NONE,
            EXPR_INVALID,
            EXPR_TYPE,
            EXPR_IDENTIFIER,
            EXPR_LITERAL,
            EXPR_UNARY,
            EXPR_BINARY,
            EXPR_TERNARY,
            
            EXPR_PARAMETER,
            EXPR_FUNCTION,
            EXPR_CLOSURE,

            EXPR_CALL,

            STMT_NONE,
            STMT_INVALID,
            STMT_IF,
            STMT_WHILE,
            STMT_RETURN,
            STMT_BREAK,
            STMT_CONTINUE,

            ITEM_COMPOUND,
            STMT_COMPOUND,

            ITEM_USE,
            ITEM_MODULE,
            ITEM_DECLARATION,
            STMT_DECLARATION,
            ITEM_FUNCTION_DECLARATION,
            ITEM_TYPE_DECLARATION,

            EXPR_PROPERTY,
            EXPR_METHOD,
            EXPR_OPERATOR,
            EXPR_INITIALIZER_SET,
            EXPR_CONSTRUCTOR,
            EXPR_DESTRUCTOR,

            ITEM_STRUCT_DECLARATION,

            EXPR_ENUM_SET,
            ITEM_ENUM_DECLARATION,

            ITEM_INVALID,
        };

        using t_node_id = size_t;
        using t_node_list = std::vector<t_node_id>;
        
        struct node {
            node(const core::lisel& selection, const node_type type)
                : selection(selection), type(type) {}

            core::lisel selection;
            node_type type;
        };

        struct ast_root : node {
            ast_root()
                : node(core::lisel(0, 0), node_type::ROOT) {}

            t_node_list item_list; // item
        };

        struct expr_none : node {
            expr_none(const core::lisel& selection)
                : node(selection, node_type::EXPR_NONE) {}
        };
        
        struct expr_invalid : node {
            expr_invalid(const core::lisel& selection)
                : node(selection, node_type::EXPR_INVALID) {}
        };

        struct expr_type : node {
            enum class e_reference_type : uint8_t {
                NONE,
                LVALUE,
                RVALUE
            };

            expr_type(const core::lisel& selection, const t_node_id source, t_node_list&& argument_list, const bool is_const, const bool is_pointer, const e_reference_type reference_type)
                : node(selection, node_type::EXPR_TYPE), source(source), argument_list(std::move(argument_list)), is_const(is_const), is_pointer(is_pointer), reference_type(reference_type) {}
            
            t_node_id source; // expr_identifier | expr_binary (scope_resolution) 
            t_node_list argument_list; // expr
            
            bool is_const;
            bool is_pointer;
            e_reference_type reference_type;
        };

        // Get identifier contents by observing its selection in the source code.
        struct expr_identifier : node {
            expr_identifier(const core::lisel& selection, const t_identifier_id id)
                : node(selection, node_type::EXPR_IDENTIFIER), id(id) {}

            expr_identifier(const core::lisel& selection, liprocess& process)
                : node(selection, node_type::EXPR_IDENTIFIER), id(process.identifier_lookup.insert(process.sub_source_code(selection))) {}
                
            t_identifier_id id;

            inline const std::string read(liprocess& process) const {
                return process.identifier_lookup.get(id);
            }
        };

        struct expr_literal : node {
            enum class e_literal_type : uint8_t {
                FLOAT,
                INT,
                STRING,
                CHAR,
                BOOL,
                NIL,
            };

            expr_literal(const core::lisel& selection, const e_literal_type literal_type)
                : node(selection, node_type::EXPR_LITERAL), literal_type(literal_type) {}

            e_literal_type literal_type;
        };

        struct expr_unary : node {
            expr_unary(const core::lisel& selection, t_node_id operand, const core::token& opr, const bool post)
                : node(selection, node_type::EXPR_UNARY), operand(operand), opr(opr), post(post) {}

            t_node_id operand; // expr
            core::token opr;
            bool post;
        };

        /*
        
        Important syntactic checks that should be implemented sooner or later (preferrably sooner)
            - SCOPE RESOLUTION NODES CAN ONLY HAVE IDENTIFIERS. NOT EVEN PARANETHESIZED EXPRESSIONS
            - MEMBER ACCESS RHS MEMBERS SHOULD ONLY BE IDENTIFIERS. PARENTHESIZED EXPRESSIONS SHOULD BE ALLOWED
            - yada dada do, you get the point future me

            13 oct 2025
        
        */

        struct expr_binary : node {
            expr_binary(const core::lisel& selection, t_node_id first, t_node_id second, const core::token& opr)
                : node(selection, node_type::EXPR_BINARY), first(first), second(second), opr(opr) {}

            t_node_id first; // expr
            t_node_id second; // expr
            core::token opr;
        };
        
        struct expr_ternary : node {
            expr_ternary(const core::lisel& selection, t_node_id first, t_node_id second, t_node_id third)
                : node(selection, node_type::EXPR_TERNARY), first(first), second(second), third(third) {}

            t_node_id first; // expr
            t_node_id second; // expr
            t_node_id third; // expr
        };
        
        struct expr_parameter : node {
            expr_parameter(const core::lisel& selection, t_node_id name, t_node_id default_value, t_node_id value_type)
                : node(selection, node_type::EXPR_PARAMETER), name(name), default_value(default_value), value_type(value_type) {}

            t_node_id name; // expr_identifier
            t_node_id default_value; // expr?
            t_node_id value_type; // expr_type?
        };

        struct expr_function : node {
            expr_function(const core::lisel& selection, t_node_list&& template_parameter_list, t_node_list&& parameter_list, t_node_id body, t_node_id return_type)
                : node(selection, node_type::EXPR_FUNCTION), template_parameter_list(std::move(template_parameter_list)), parameter_list(std::move(parameter_list)), body(body), return_type(return_type) {}

            t_node_list template_parameter_list; // expr_identifier - later will become expr_template_param
            t_node_list parameter_list; // expr_parameter 
            t_node_id body; // stmt
            t_node_id return_type; // expr_type?
        };

        struct expr_call : node {
            expr_call(const core::lisel& selection, t_node_id callee, t_node_list&& template_argument_list, t_node_list&& argument_list)
                : node(selection, node_type::EXPR_CALL), callee(callee), template_argument_list(std::move(template_argument_list)), argument_list(std::move(argument_list)) {}

            t_node_id callee; // expr_identifier | expr_binary (scope_resolution) 
            t_node_list template_argument_list; // expr
            t_node_list argument_list; // expr
        };
        
        struct stmt_none : node {
            stmt_none(const core::lisel& selection)
                : node(selection, node_type::STMT_NONE) {}
        };

        struct stmt_invalid : node {
            stmt_invalid(const core::lisel& selection)
                : node(selection, node_type::STMT_INVALID) {}
        };

        struct stmt_if : node {
            stmt_if(const core::lisel& selection, t_node_id condition, t_node_id consequent, t_node_id alternate)
                : node(selection, node_type::STMT_IF), condition(condition), consequent(consequent), alternate(alternate) {}

            t_node_id condition; // expr
            t_node_id consequent; // stmt
            t_node_id alternate; // stmt?
        };
        
        struct stmt_while : node {
            stmt_while(const core::lisel& selection, t_node_id condition, t_node_id consequent, t_node_id alternate)
                : node(selection, node_type::STMT_WHILE), condition(condition), consequent(consequent), alternate(alternate) {}

            t_node_id condition; // expr
            t_node_id consequent; // stmt
            t_node_id alternate; // stmt?
        };
        
        struct stmt_return : node {
            stmt_return(const core::lisel& selection, t_node_id expression)
                : node(selection, node_type::STMT_RETURN), expression(expression) {}
            
            t_node_id expression; // expr?
        };
        
        struct item_compound : node {
            item_compound(const core::lisel& selection, t_node_list&& item_list)
                : node(selection, node_type::ITEM_COMPOUND), item_list(std::move(item_list)) {}

            t_node_list item_list; // item
        };

        struct stmt_compound : node {
            stmt_compound(const core::lisel& selection, t_node_list&& stmt_list)
                : node(selection, node_type::STMT_COMPOUND), stmt_list(std::move(stmt_list)) {}

            t_node_list stmt_list; // stmt
        };

        struct stmt_break : node {
            stmt_break(const core::lisel& selection)
                : node(selection, node_type::STMT_BREAK) {}
        };

        struct stmt_continue : node {
            stmt_continue(const core::lisel& selection)
                : node(selection, node_type::STMT_CONTINUE) {}
        };

        struct item_use : node {
            item_use(const core::lisel& selection, t_node_id path)
                : node(selection, node_type::ITEM_USE), path(path) {}

            t_node_id path; // expr_literal (string)
        };

        struct item_module : node {
            item_module(const core::lisel& selection, t_node_id source, t_node_id content)
                : node(selection, node_type::ITEM_MODULE), source(source), content(content) {}

            t_node_id source; // expr_identifier | expr_binary (scope_resolution)
            t_node_id content; // item
        };

        // Can be wrapped into a statement.
        struct stmt_declaration : node {
            stmt_declaration(const core::lisel& selection, t_node_id name, t_node_id value_type, t_node_id value)
                : node(selection, node_type::STMT_DECLARATION), name(name), value_type(value_type), value(value) {}
            
            t_node_id name; // expr_identifier
            t_node_id value_type; // expr_type?
            t_node_id value; // expr
        };

        struct item_declaration : node {
            item_declaration(const core::lisel& selection, t_node_id source, t_node_id value_type, t_node_id value)
                : node(selection, node_type::ITEM_DECLARATION), source(source), value_type(value_type), value(value) {}
            
            t_node_id source; // expr_identifier | expr_binary (scope_resolution)
            t_node_id value_type; // expr_type?
            t_node_id value; // expr
        };

        struct item_function_declaration : node {
            item_function_declaration(const core::lisel& selection, t_node_id source, t_node_id function)
                : node(selection, node_type::ITEM_FUNCTION_DECLARATION), source(source), function(function) {}

            t_node_id source; // expr_identifier | expr_binary (scope_resolution)
            t_node_id function; // expr_function 
        };

        struct item_type_declaration : node {
            item_type_declaration(const core::lisel& selection, t_node_id source, t_node_id type_value, t_node_list&& template_parameter_list)
                : node(selection, node_type::ITEM_TYPE_DECLARATION), source(source), type_value(type_value), template_parameter_list(std::move(template_parameter_list)) {}
            
            t_node_id source; // expr_identifier | expr_binary (scope_resolution)
            t_node_id type_value; // expr_type
            t_node_list template_parameter_list; // expr_identifier
        };

            struct expr_property : node {
                expr_property(const core::lisel& selection, const t_node_id name, const t_node_id value_type, const t_node_id default_value, const bool is_private)
                    : node(selection, node_type::EXPR_PROPERTY), name(name), value_type(value_type), default_value(default_value), is_private(is_private) {}

                t_node_id name; // expr_identifier
                t_node_id value_type; // expr_type?
                t_node_id default_value; // expr?

                bool is_private;
            };

            struct expr_method : node {
                expr_method(const core::lisel& selection, const t_node_id name, const t_node_id function, const bool is_private, const bool is_const)
                    : node(selection, node_type::EXPR_METHOD), name(name), function(function), is_private(is_private), is_const(is_const) {}

                t_node_id name; // expr_identifier
                t_node_id function; // expr_function

                bool is_private;
                bool is_const;
            };
            
            struct expr_operator : node {
                expr_operator(const core::lisel& selection, const core::token_type opr, const t_node_id function, const bool is_const)
                    : node(selection, node_type::EXPR_OPERATOR), opr(opr), function(function), is_const(is_const) {}
                    
                core::token_type opr;
                t_node_id function; // expr_function
                bool is_const;
            };

            struct expr_initializer_set : node {
                expr_initializer_set(const core::lisel& selection, const t_node_id property_name, const t_node_id value)
                    : node(selection, node_type::EXPR_INITIALIZER_SET), property_name(property_name), value(value) {}

                t_node_id property_name; // expr_identifier
                t_node_id value; // expr
            };

            struct expr_constructor : node {
                expr_constructor(const core::lisel& selection, const t_node_id name, const t_node_id function, t_node_list&& initializer_list)
                    : node(selection, node_type::EXPR_CONSTRUCTOR), name(name), function(function), initializer_list(std::move(initializer_list)) {}
                
                t_node_id name; // expr_identifier
                t_node_id function; // expr_function
                t_node_list initializer_list; // expr_initializer_set
            };

            struct expr_destructor : node {
                expr_destructor(const core::lisel& selection, const t_node_id body)
                    : node(selection, node_type::EXPR_DESTRUCTOR), body(body) {}

                t_node_id body; // stmt
            };

        struct item_struct_declaration : node {
            item_struct_declaration(const core::lisel& selection, const t_node_id source, t_node_list&& template_parameter_list, t_node_list&& member_list)
                : node(selection, node_type::ITEM_STRUCT_DECLARATION), source(source), template_parameter_list(std::move(template_parameter_list)), member_list(std::move(member_list)) {}

            t_node_id source; // expr_identifier | expr_binary (scope_resolution)

            t_node_list template_parameter_list; // expr_identifier
            t_node_list member_list; // struct_member | operator_overload | constructor | destructor
        };

        
        struct expr_enum_set : node {
            expr_enum_set(const core::lisel& selection, const t_node_id name, const t_node_id value)
                : node(selection, node_type::EXPR_ENUM_SET), name(name), value(value) {}

            t_node_id name; // expr_identifier
            t_node_id value; // expr_literal (int)?
        };
        
        struct item_enum_declaration : node {
            item_enum_declaration(const core::lisel& selection, const t_node_id source, t_node_list&& set_list)
                : node(selection, node_type::ITEM_ENUM_DECLARATION), source(source), set_list(std::move(set_list)) {}

            t_node_id source; // expr_identifier | expr_binary (scope_resolution)
            t_node_list set_list; // expr_enum_set | expr_none
        };

        struct item_invalid : node {
            item_invalid(const core::lisel& selection)
                : node(selection, node_type::ITEM_INVALID) {}
        };

        struct arena_node {
            template <typename T>
            arena_node(T&& node)
                : _raw(std::forward<T>(node)) {}

            std::variant<
                ast_root,

                expr_none,
                expr_invalid,
                expr_type,
                expr_identifier,
                expr_literal,
                expr_unary,
                expr_binary,
                expr_ternary,
                expr_parameter,
                expr_function,
                expr_call,

                stmt_none,
                stmt_invalid,
                stmt_if,
                stmt_while,
                stmt_return,
                item_compound,
                stmt_compound,
                stmt_break,
                stmt_continue,

                item_use,
                item_module,
                item_declaration,
                stmt_declaration,
                item_function_declaration,
                item_type_declaration,

                expr_property,
                expr_method,
                expr_operator,
                expr_initializer_set,
                expr_constructor,
                expr_destructor,
                item_struct_declaration,
                expr_enum_set,
                item_enum_declaration,
                
                item_invalid
            > _raw;
        };

        struct ast_arena : liutil::arena<t_node_id, node, arena_node> {
            void pretty_debug(const liprocess& process, const t_node_id id, std::string& buffer, uint8_t indent = 0);
            bool is_expression_wrappable(const t_node_id id);
        };
    }
}