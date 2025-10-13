#include <string>

#include "core.hh"
#include "ast.hh"
#include "symbol.hh"

using namespace core::sym;
using namespace core::ast;

struct semantic_state {
    semantic_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process),
          file_id(file_id),
          file(process.file_list[file_id]),
          ast_arena(std::any_cast<const core::ast::ast_arena&>(process.file_list[file_id].dump_ast_arena))
          {}

    core::liprocess& process;

    const core::t_file_id file_id;
    core::liprocess::lifile& file;

    const core::ast::ast_arena& ast_arena;
    symbol_arena arena;

    /*
    
    Semantic analysis flags and state

    */ 

        // The current module that all newly generated items should be appended to.
        t_symbol_id focused_module_id;

        // If true, then any statement being parsed will not check typenames that match to a template parameter.
        bool ignore_template_content = false;

        // The active function that is being parsed. Is valid to all statement parsing functions.
        expr_function* active_function;

        // If all runs of the analyzer are successful - No errors thrown.
        bool semantic_success = true;

    /*
    
    ====================================================
    
    */

    template <typename T>
    inline const T& get_node(const t_node_id id) const {
        return ast_arena.get_as<T>(id);
    }

    inline const core::ast::node* get_node_base_ptr(const t_node_id id) const {
        return ast_arena.get_base_ptr(id);
    }

    template <typename T>
    inline T& get_symbol(t_symbol_id id) {
        return arena.get_as<T>(id);
    }

    inline void add_log(const core::lilog::log_level log_level, const core::lisel& selection, const std::string& message) {
        process.add_log(log_level, selection, message);

        if (log_level == core::lilog::log_level::ERROR || log_level == core::lilog::log_level::COMPILER_ERROR) {
            semantic_success = false;
        }        
    }
};

struct semantic_subthread {

};

/*

====================================================

*/

static void eval_expr(semantic_state& state, const t_node_id node_id);
static void eval_stmt(semantic_state& state, const t_node_id node_id);
static void eval_item(semantic_state& state, const core::ast::t_node_id node_id);

/*

====================================================

*/

// Ensure that resolution_node's operator is correct
static t_symbol_id search_symbol(semantic_state& state, const expr_binary& resolution_node) {
    UNREACHABLE();
}

// Ensure that resolution_node's operator is correct
static std::pair<decl_module&, core::t_identifier_id> search_symbol_for_naming(semantic_state& state, const expr_binary& resolution_node) {
    // First check from the perspective of the current module

    // If the entry point identifier is not recognizable, 
    UNREACHABLE();
}

// If a variable has no explicit type, see if its value can be deduced.
// Same can go with parameters and other expressions.
// NOTE: This function does not type check the literal.
static t_symbol_id deduce_expression_type(semantic_state& state, const t_node_id expr_node_id) {
    const node* node_base_ptr = state.get_node_base_ptr(expr_node_id);

    switch (node_base_ptr->type) {
        // Top level variables
        case node_type::EXPR_IDENTIFIER:

        // Track down function path and check return value
        case node_type::EXPR_CALL:

        // Member access or scope resolution
        case node_type::EXPR_BINARY:

        case node_type::EXPR_LITERAL:

        case node_type::EXPR_TERNARY: {

        }
        case node_type::EXPR_UNARY: {
            const t_node_id operand_id = ((expr_unary*)node_base_ptr)->operand;
            return deduce_expression_type(state, operand_id);
        }
        default:
            state.add_log(
                core::lilog::log_level::COMPILER_ERROR,
                node_base_ptr->selection,
                "Attempted to deduce a type that can not be deduced. [AST NODE TYPE: " + std::to_string((uint8_t)node_base_ptr->type) + "]"
            );
        
    }
    return 0;
}

// Covers all declarations. Appends basically any symbol into the currently active namespaces.
static void append_declaration(semantic_state& state, const core::lisel& selection, const t_node_id resolution_node_id, const t_symbol_id symbol_id) {
    auto& focused_module = state.get_symbol<decl_module>(state.focused_module_id);

    // Locate the module we want to declare the variable in RELATIVE TO focused_module

    std::pair<decl_module&, core::t_identifier_id> name_info = search_symbol_for_naming(state, state.get_node<expr_binary>(resolution_node_id));
    auto& target_module = name_info.first;
    auto& name_id = name_info.second;

    if (target_module.declarations.find(name_id) != target_module.declarations.end()) {
        state.add_log(core::lilog::log_level::ERROR, selection, "\"" + state.process.identifier_lookup.get(name_id) + "\" was already declared.");
        return;
    }

    target_module.declarations[name_id] = symbol_id;
}

/*

====================================================

*/

// Local variable declaration
static void eval_stmt_declaration(semantic_state& state, const stmt_declaration& focus_node) {

}

// Global variable declaration
static void eval_item_declaration(semantic_state& state, const item_declaration& focus_node) {
    append_declaration(
        state,
        focus_node.selection,
        focus_node.source,
        state.arena.insert(decl_variable(focus_node.value_type))
    );

    eval_expr(state, focus_node.value);
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& focus_node) {
    const expr_function& func_node = state.get_node<expr_function>(focus_node.function);

    append_declaration(
        state,
        focus_node.selection,
        focus_node.source,
        state.arena.insert(decl_function(state.ast_arena, func_node))
    );

    // Veryify/test the return and parameter types.
    for (const t_node_id parameter : func_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter);

        t_symbol_id type_symbol;

        const node_type parameter_value_type = state.get_node_base_ptr(parameter_node.value_type)->type;
        const core::ast::node* default_value_base_node_ptr = state.get_node_base_ptr(parameter_node.default_value);

        if (parameter_value_type == node_type::EXPR_NONE) {
            type_symbol = deduce_expression_type(state, parameter_node.default_value);
        }

    }

    // Run the body.
    state.ignore_template_content = true;
    state.active_function = const_cast<expr_function*>(&func_node);

    eval_stmt(state, func_node.body);
}

static void eval_item_module(semantic_state& state, const item_module& focus_node) {
    t_symbol_id parent_module_id = state.focused_module_id;
    t_symbol_id new_module_id = state.arena.insert(decl_module());

    state.focused_module_id = new_module_id;
    eval_item(state, focus_node.content);
    state.focused_module_id = parent_module_id;
}

static void eval_expr(semantic_state& state, const t_node_id node_id) {
    const core::ast::node* base_ptr = state.get_node_base_ptr(node_id); 

    switch (base_ptr->type) {
        default:
            break;
    }
}

static void eval_stmt(semantic_state& state, const t_node_id node_id) {
    const core::ast::node* node_base_ptr = state.get_node_base_ptr(node_id); 
    
    switch (node_base_ptr->type) {
        case core::ast::node_type::STMT_DECLARATION:
            eval_expr_declaration(state, *(stmt_declaration*)node_base_ptr);
            break;
        default:
            break;
    }
}

static void eval_item(semantic_state& state, const core::ast::t_node_id node_id) {
    const core::ast::node* node_base_ptr = state.get_node_base_ptr(node_id); 
    
    switch (node_base_ptr->type) {
        case core::ast::node_type::ITEM_MODULE:
            eval_item_module(state, *(core::ast::item_module*)node_base_ptr);
            break;
        case core::ast::node_type::ITEM_DECLARATION:
            eval_item_declaration(state, *(core::ast::item_declaration*)node_base_ptr);
            break;
        case core::ast::node_type::ITEM_FUNCTION_DECLARATION:
            eval_item_function_declaration(state, *(core::ast::item_function_declaration*)node_base_ptr);
            break;
        default:
            break;
    }

    state.process.add_log(
        core::lilog::log_level::COMPILER_ERROR,
        node_base_ptr->selection,
        "Unexpected AST node - Expected 'item', got [code " + std::to_string((uint8_t)node_base_ptr->type) + "])"
    );
}

static void eval_ast_root(semantic_state& state, const core::ast::ast_root& node) {
    state.arena.insert(sym_root());
    state.focused_module_id = 1;

    for (const core::ast::t_node_id& child : node.item_list) {
        eval_item(state, child);
    }
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);

    eval_ast_root(state, state.get_node<core::ast::ast_root>(0));

    return true;
}