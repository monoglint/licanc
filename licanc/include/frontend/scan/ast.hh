#pragma once

/*

INSTRUCTIONS FOR MODDING

Add a new t_ast_node_type value. Create a new struct that inherits from t_node.
Append the new struct type to the variant at the bottom of the file.

note: only have ast nodes inherit from decl, stmt, or expr if they can be guaranteed to be one. by that, imagine
a function argument ast node. its value can be any type of expression. only label an ast node as an expr if you want
it to be passable as a funciton argument

*/

#include <variant>
#include <type_traits>

#include "util/arena.hh"

#include "frontend/scan/token.hh"

#include "util/span.hh"
#include "util/safe_id.hh"

#include "frontend/manager_types.hh"

namespace frontend::scan::ast {
    struct t_node {
        t_node(util::t_span span)
            : span(std::move(span)) 
        {}

        util::t_span span;
    };

    enum class t_decl_type {
        IMPORT,
        GLOBAL,
        FUNCTION,
        RECORD,
        MODULE,
    };

    struct t_decl : t_node {
        t_decl(util::t_span span, t_decl_type decl_type)
            : t_node(span), decl_type(decl_type) 
        {}

        t_decl_type decl_type;
    };

    enum class t_stmt_type {
        RETURN,
        COMPOUND,
    };
        
    struct t_stmt : t_node {
        t_stmt(util::t_span span, t_stmt_type stmt_type)
            : t_node(span), stmt_type(stmt_type) 
        {}

        t_stmt_type stmt_type;
    };

    enum class t_expr_type {
        IDENTIFIER,
        STRING_LITERAL,
        UNARY,
        BINARY,
        SCOPE_RESOLUTION,
        TERNARY,
        SCOPE_REFERENCE,
        CALL,
    };

    struct t_expr : t_node {
        t_expr(util::t_span span, t_expr_type expr_type)
            : t_node(span), expr_type(expr_type) 
        {}

        t_expr_type expr_type;
    };
    
    template <typename T>
    using t_ptrs = std::vector<T*>;

    using t_node_ptrs = t_ptrs<t_node>;
    using t_decl_ptrs = t_ptrs<t_decl>;
    using t_stmt_ptrs = t_ptrs<t_stmt>;
    using t_expr_ptrs = t_ptrs<t_expr>;

    //            ||||||
    //            ||||||
    // real nodes vvvvvv
    //
    //
    struct t_root : t_node {
        t_root()
            : t_node({}) 
        {}

        t_decl_ptrs decls;
    };

    // x
    struct t_identifier : t_expr {
        t_identifier(util::t_span span, manager::t_identifier_id identifier_id)
            : t_expr(std::move(span), t_expr_type::IDENTIFIER), identifier_id(identifier_id) 
        {}
        
        // reference to within manager::t_compilation_unit
        manager::t_identifier_id identifier_id;
    };

    // "hello world"
    struct t_string_literal : t_expr {
        t_string_literal(util::t_span span, manager::t_string_literal_id string_literal_id)
            : t_expr(std::move(span), t_expr_type::STRING_LITERAL), string_literal_id(string_literal_id) 
        {}

        manager::t_string_literal_id string_literal_id;
    };

    // struct t_number_literal : t_node {
    //     t_number_literal(util::t_span span, manager::t_number_literal_id number_literal_id, token::t_token_type suffix)
    //         : t_node(std::move(span)), number_literal_id(number_literal_id), suffix(suffix) {}

    //     manager::t_number_literal_id number_literal_id;
    //     token::t_token_type suffix;
    // };
    // ^^^^^^^^ update how number literals are stored during scan time before re-enabling this

    // -5
    struct t_unary_expr : t_expr {
        t_unary_expr(util::t_span span, t_expr* operand, token::t_token_type opr)
            : t_expr(std::move(span), t_expr_type::UNARY), operand(operand), opr(opr) 
        {}

        t_expr* operand;
        token::t_token_type opr;
    };

    // 5 + 2
    struct t_binary_expr : t_expr {
        t_binary_expr(util::t_span span, t_expr* operand0, t_expr* operand1, token::t_token_type opr)
            : t_expr(std::move(span), t_expr_type::BINARY), operand0(operand0), operand1(operand1), opr(opr) 
        {}

        t_expr* operand0;
        t_expr* operand1;
        token::t_token_type opr;
    };

    struct t_template_argument;
    struct t_scope_reference_expr;
    // math..pi
    struct t_scope_resolution_expr : t_expr {
        t_scope_resolution_expr(util::t_span span, t_scope_reference_expr* operand0, t_identifier* operand1, std::vector<t_template_argument*> template_arguments = {})
            : t_expr(std::move(span), t_expr_type::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1), template_arguments(std::move(template_arguments))
         {}

        t_scope_reference_expr* operand0;
        t_identifier* operand1;

        std::vector<t_template_argument*> template_arguments; // {t_template_argument}
    };

    // a > b ? x : y
    struct t_ternary_expr : t_expr {
        t_ternary_expr(util::t_span span, t_expr* condition, t_expr* consequent, t_expr* alternate, token::t_token_type opr)
            : t_expr(std::move(span), t_expr_type::TERNARY), condition(condition), consequent(consequent), alternate(alternate), opr(opr) 
        {}

        t_expr* condition;
        t_expr* consequent;
        t_expr* alternate;
        token::t_token_type opr;
    };

    using t_scope_reference_variant = std::variant<t_identifier*, t_scope_resolution_expr*>;
    // a::b() || a()
    struct t_scope_reference_expr : t_expr {
        t_scope_reference_expr(util::t_span span, t_scope_reference_variant reference)
            : t_expr(std::move(span), t_expr_type::SCOPE_REFERENCE), reference(std::move(reference)) 
        {}

        t_scope_reference_variant reference;
    };

    /*
    
    templated initializer in templated struct scenario
    util..reference_wrapper<int>..new<float>()

    */

    struct t_call_expr : t_expr {
        t_call_expr(util::t_span span, t_scope_reference_expr* callee, t_expr_ptrs arguments = {}, std::vector<t_template_argument*> template_arguments = {})
            : t_expr(std::move(span), t_expr_type::CALL), callee(callee), arguments(std::move(arguments)), template_arguments(std::move(template_arguments)) 
        {}

        t_scope_reference_expr* callee; // t_scope_reference
        t_expr_ptrs arguments; // {t_expr}
        std::vector<t_template_argument*> template_arguments;
    };

    struct t_type;
    using t_type_source_variant = std::variant<t_type*, t_scope_reference_expr*>;
    // array<u8>
    struct t_type : t_node {
        t_type(util::t_span span, t_type_source_variant source, std::vector<t_template_argument*> template_arguments, token::t_token_type qualifier)
            : t_node(std::move(span)), source(std::move(source)), template_arguments(std::move(template_arguments)), qualifier(qualifier) 
        {}

        t_type_source_variant source;
        std::vector<t_template_argument*> template_arguments;

        token::t_token_type qualifier;
    };

    /*

    ; file: "color.li"
    struct color {
        r: u8 = 0
        g: u8 = 0
        b: u8 = 0
    }

    ; file: "main.li"
    import "io.li" ; if io has its own module, then that module will be dumped into the global module here
    
    module art {
        import "color.li" ; imports can be oranized into local modules
    }

    art..color()
    
    */

    // import "math"
    struct t_import_decl : t_decl {
        t_import_decl(util::t_span span, t_string_literal* file_path, t_string_literal* absolute_file_path, bool is_path_valid)
            : t_decl(std::move(span), t_decl_type::IMPORT), file_path(file_path), absolute_file_path(absolute_file_path), is_path_valid(is_path_valid) 
        {}

        t_string_literal* file_path;
        t_string_literal* absolute_file_path;
        bool is_path_valid;
        /* manager.cc */ manager::t_file_id resolved_file_id; // proper dependency connection is made here
    };

    // 
    struct t_global_decl : t_decl {
        t_global_decl(util::t_span span, t_identifier* name, t_type* type, t_expr* value)
            : t_decl(std::move(span), t_decl_type::GLOBAL), name(name), type(type), value(value) 
        {}

        t_identifier* name;
        t_type* type; // t_type
        t_expr* value; // expr
    };

    struct t_type_name_template_parameter : t_node {
        t_identifier* name;
    };

    struct t_value_template_parameter : t_node {
        t_identifier* name;
        t_type* type;
    };

    using t_template_parameter_variant = std::variant<t_type_name_template_parameter*, t_value_template_parameter*>;
    struct t_template_parameter : t_node {
        t_template_parameter(util::t_span span, t_template_parameter_variant value)
            : t_node(std::move(span)), value(std::move(value)) 
        {}

        t_template_parameter_variant value;
    };

    using t_template_argument_variant = std::variant<t_type*, t_expr*>;
    struct t_template_argument : t_node {
        t_template_argument(util::t_span span, t_template_argument_variant value)
            : t_node(std::move(span)), value(std::move(value)) 
        {}

        // note: if value is a t_expr, then the value of the template argument must be proven to be a compile time constant
        t_template_argument_variant value;
    };

    struct t_function_parameter : t_node {
        t_function_parameter(util::t_span span, t_identifier* name, t_type* type)
            : t_node(std::move(span)), name(name), type(type) 
        {}

        t_identifier* name;
        t_type* type;
    };
    
    struct t_function : t_node {
        t_function(util::t_span span, std::vector<t_function_parameter*> parameters, t_stmt* body, t_type* return_type)
            : t_node(std::move(span)), parameters(std::move(parameters)), body(body), return_type(return_type) 
        {}

        std::vector<t_function_parameter*> parameters;
        t_stmt* body;
        t_type* return_type;
    };

    struct t_function_template : t_node {
        t_function_template(util::t_span span, t_function* base, std::vector<t_template_parameter*> template_parameters)
            : t_node(std::move(span)), base(base), template_parameters(std::move(template_parameters)) 
        {}

        t_function* base;
        std::vector<t_template_parameter*> template_parameters; // {t_template_parameter}
    };
    
    struct t_function_decl : t_decl {
        t_function_decl(util::t_span span, t_function_template* function_template, t_identifier* name)
            : t_decl(std::move(span), t_decl_type::FUNCTION), function_template(function_template), name(name) 
            {}

        t_function_template* function_template;
        t_identifier* name;
    };

    struct t_initializer : t_node {
        t_initializer(util::t_span span, t_identifier* name, t_function* function)
            : t_node(std::move(span)), name(name), function(function) 
        {}

        t_identifier* name; // t_identifier
        t_function* function; // NO DEPENDENT TYPES
    };

    struct t_finalizer : t_node {
        t_finalizer(util::t_span span, t_function* function)
            : t_node(std::move(span)), function(function) 
        {}

        t_function* function; // NO DEPENDENT TYPES
    };

    struct t_method : t_node {
        t_method(util::t_span span, token::t_token_type access_specifier, t_identifier* name, t_function_template* function_template)
            : t_node(std::move(span)), access_specifier(access_specifier), name(name), function_template(function_template) 
        {}

        token::t_token_type access_specifier;
        t_identifier* name;
        t_function_template* function_template;
    };

    struct t_property : t_node {
        t_property(util::t_span span, token::t_token_type access_specifier, t_identifier* name, t_type* type)
            : t_node(std::move(span)), access_specifier(access_specifier), name(name), type(type) 
        {}

        token::t_token_type access_specifier;
        t_identifier* name;
        t_type* type;
    };

    struct t_record : t_node {
        t_record(util::t_span span, std::vector<t_method*> methods, std::vector<t_property*> properties, std::vector<t_initializer*> initializers, t_finalizer* finalizer)
            : t_node(std::move(span)), methods(std::move(methods)), properties(std::move(properties)), initializers(std::move(initializers)), finalizer(finalizer) 
        {}

        std::vector<t_method*> methods;
        std::vector<t_property*> properties;
        std::vector<t_initializer*> initializers;
        t_finalizer* finalizer; // t_finalizer?
    };

    struct t_record_template : t_node {
        t_record_template(util::t_span span, t_record* base, std::vector<t_template_parameter*> template_parameters)
            : t_node(std::move(span)), base(base), template_parameters(std::move(template_parameters)) 
        {}

        t_record* base;
        std::vector<t_template_parameter*> template_parameters;
    };

    struct t_record_decl : t_decl {
        t_record_decl(util::t_span span, t_record_template* record_template, t_identifier* name)
            : t_decl(std::move(span), t_decl_type::RECORD), record_template(record_template), name(name) 
        {}

        t_record_template* record_template;
        t_identifier* name;
    };

    // struct t_type_alias_template : t_node {
    //     t_type_alias_template(util::t_span span, t_type* type, std::vector<t_type*> instantiations, std::vector<t_template_parameter*> template_parameters)
    //         : t_node(std::move(span)), type(type), instantiations(std::move(instantiations)), template_parameters(std::move(template_parameters)) {}

    //     t_type* type;
    //     std::vector<t_type*> instantiations;
    //     std::vector<t_template_parameter*> template_parameters;
    // };

    // struct t_type_alias_decl : t_item {
    //     t_type_alias_decl(util::t_span span, t_type_alias_template* type_alias_template, t_identifier* name)
    //         : t_item(std::move(span)), type_alias_template(type_alias_template), name(name) {}

    //     t_type_alias_template* type_alias_template;
    //     t_identifier* name;
    // };

    struct t_module_decl : t_decl {
        t_module_decl(util::t_span span, t_identifier* name, t_decl_ptrs decls)
            : t_decl(std::move(span), t_decl_type::MODULE), name(name), decls(std::move(decls)) 
        {}
        
        t_identifier* name; // t_identifier
        t_decl_ptrs decls; // {t_item}

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct t_return_stmt : t_stmt {
        t_return_stmt(util::t_span span, t_expr* value)
            : t_stmt(std::move(span), t_stmt_type::RETURN), value(value) 
        {}

        t_expr* value;
    };

    struct t_compound_stmt : t_stmt {
        t_compound_stmt(util::t_span span, t_stmt_ptrs stmts)
            : t_stmt(std::move(span), t_stmt_type::COMPOUND), stmts(std::move(stmts)) 
        {}

        t_stmt_ptrs stmts;
    };

    //
    //
    //

    struct t_ast {
        t_ast() {}

        // if an import node is allocated into the arena, its important to add it to the "imports" vector
        
    private:
        util::t_arena<> arena;

    public:
        t_root* root_ptr;
        std::vector<t_import_decl*> import_nodes;
        
        template <std::derived_from<t_node> T, typename... ARGS>
        T* emplace(ARGS... args);

        template <std::derived_from<t_node> T>
        inline T* push(T sym) {
            return emplace<T, T>(std::move(sym));
        }

        inline void clear() {
            arena.clear();
        }
    };
}