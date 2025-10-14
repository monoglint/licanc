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
    
    std::vector<sym_call_frame> call_frame_stack;

    /*
    
    Semantic analysis flags and state

    */ 

        // The current module that all newly generated items should be appended to.
        t_symbol_id focused_module_id;

        // If true, any template specifications will be not considered. Any call expressions will not push a new scope. No declarations will be processed.
        bool function_prescan_mode = false;

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

Helper Functions

====================================================


*/

// - type checker literally disabled lmao
// NOTE: This function will need to specify types if they possess type parameters. Beware!
// Call asert_types_match() if you're only running to throw an error.
static bool types_match(semantic_state& state, const t_symbol_id type0, const t_symbol_id type1) {
    return true;
}

// Potentially add casting later? Not necessary.
static bool assert_types_match(semantic_state& state, const core::lisel& error_selection, const t_symbol_id type0, const t_symbol_id type1) {
    if (!types_match(state, type0, type1)) {
        // !NOTE - We could totally add a flag that makes the type checker optional. Holy C community would love this
        state.add_log(core::lilog::log_level::ERROR, error_selection, "Types do not match. Double check sources and qualifiers.");
        return false;
    }

    return true;
}

// Ensure that resolution_node's operator is correct.
static t_symbol_id _search_symbol_internal(semantic_state& state, const decl_module& current_module, const t_node_id resolution_node_id) {
    const node* current_node = state.get_node_base_ptr(resolution_node_id);
    
    /*
    
    "std::chrono::time_point"


          __ :: __
         /        \
        ::     time_point
      /   \      
    std chrono
    
    */

    if (current_node->type == node_type::EXPR_IDENTIFIER) {
        const auto& identifier_node = state.get_node<expr_identifier>(resolution_node_id);

        if (current_module.has_item(identifier_node.id)) {
            return current_module.declaration_map.at(identifier_node.id);
        }

        // Quick global-namespace-oriented fallback.
        auto& global_module = state.get_symbol<decl_module>(state.arena.get_as<sym_root>(0).global_module);
        
        if (global_module.has_item(identifier_node.id)) {
            return global_module.declaration_map.at(identifier_node.id);
        }

        // Okay, fallback unsuccessful; we can confidently assert that the programmer fucked up in some way, shape, or form.
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + identifier_node.read(state.process) + "\" was not declared in this scope.");
        return state.arena.insert(sym_invalid());
    }

    // If its's not an identifier, then it is a binary expression.

    const auto& expr_binary_current_node = *(expr_binary*)current_node;
    decl_module* focused_module; 
    
    // Resolve LHS
        // If this condition is true, we're on the very very bottom left of the diagram.
        if (state.get_node_base_ptr(expr_binary_current_node.first)->type == node_type::EXPR_IDENTIFIER)
            focused_module = &state.get_symbol<decl_module>(_search_symbol_internal(state, current_module, expr_binary_current_node.first));

        // Otherwise, the parse has syntactically guaranteed that the left side will be another binary expression.
        else {
            const t_symbol_id resolved_lhs = _search_symbol_internal(state, current_module, expr_binary_current_node.first);
            
            if (state.arena.get_base_ptr(resolved_lhs)->type != symbol_type::DECL_MODULE) {
                state.add_log(core::lilog::log_level::ERROR, current_node->selection, "Attempted to search inside a symbol that was not a module.");
                return state.arena.insert(sym_invalid());
            }

            focused_module = &state.get_symbol<decl_module>(resolved_lhs);
        }

    // Resolve RHS

    const expr_identifier& rhs_identifier_node = state.get_node<expr_identifier>(expr_binary_current_node.second);
     
    if (!focused_module->has_item(rhs_identifier_node.id)) {
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + rhs_identifier_node.read(state.process) + "\" was not declared in this scope.");
        return state.arena.insert(sym_invalid());
    }

    return focused_module->declaration_map.at(rhs_identifier_node.id);
}

static t_symbol_id search_symbol(semantic_state& state, const t_node_id resolution_node_id) {
    return _search_symbol_internal(state, state.get_symbol<decl_module>(state.focused_module_id), resolution_node_id);
}


// Ensure that resolution_node's operator is correct
static std::pair<decl_module&, core::t_identifier_id> search_symbol_for_naming(semantic_state& state, const t_node_id resolution_node) {
    // First check from the perspective of the current module

    // If the entry point identifier is not recognizable, 
    UNREACHABLE();
}

// Locate a type in the symbol table.
static t_symbol_id eval_type(semantic_state& state, const t_node_id resolution_node_id) {
    const t_symbol_id found_symbol_id = search_symbol(state, resolution_node_id);
    const symbol* symbol_base_ptr = state.arena.get_base_ptr(found_symbol_id);
    
    switch (symbol_base_ptr->type) {
        case symbol_type::DECL_ENUM:
        case symbol_type::DECL_STRUCT:
        case symbol_type::DECL_PRIMITIVE:

        // search_symbol already casted its log if an error was made, so we'll include the invalid symbol type as a fallthrough condition.
        case symbol_type::INVALID:
            return found_symbol_id;
        default:
            state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(resolution_node_id)->selection, "Invalid symbol - not a type.");
            return state.arena.insert(sym_invalid());
    }
}

// If a variable has no explicit type, see if its value can be deduced.
// Same can go with parameters and other expressions.
// NOTE: This function does not type check the literal.
// -> decl_struct | decl_enum | primitive
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
            return 0; // start later
        }
        case node_type::EXPR_UNARY: {
            const t_node_id operand_id = ((expr_unary*)node_base_ptr)->operand;
            return deduce_expression_type(state, operand_id);
        }
        default:
            break;
        
    }

    UNREACHABLE();
}

// Covers all declarations. Appends basically any symbol into the currently active namespaces.
static void append_item_declaration(semantic_state& state, const t_node_id resolution_node_id, const t_symbol_id symbol_id) {
    // Locate the module we want to declare the variable in RELATIVE TO focused_module

    auto& resolution_node = state.get_node<expr_binary>(resolution_node_id);
    std::pair<decl_module&, core::t_identifier_id> name_info = search_symbol_for_naming(state, resolution_node_id);
    auto& target_module = name_info.first;
    auto& name_id = name_info.second;

    if (target_module.declaration_map.find(name_id) != target_module.declaration_map.end()) {
        state.add_log(core::lilog::log_level::ERROR, resolution_node.selection, "\"" + state.process.identifier_lookup.get(name_id) + "\" was already declared.");
        return;
    }

    target_module.declaration_map[name_id] = symbol_id;
}

static void append_local_declaration(semantic_state& state, const t_node_id name_node_id, const t_symbol_id symbol_id) {

}

/*

====================================================

Tree walker functions

====================================================

*/

// Local variable declaration
static void eval_stmt_declaration(semantic_state& state, const stmt_declaration& focus_node) {

}

// Global variable declaration
static void eval_item_declaration(semantic_state& state, const item_declaration& focus_node) {
    append_item_declaration(
        state,
        focus_node.source,
        state.arena.insert(decl_variable(focus_node.value_type))
    );

    eval_expr(state, focus_node.value);
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& focus_node) {
    const expr_function& func_node = state.get_node<expr_function>(focus_node.function);

    append_item_declaration(
        state,
        focus_node.source,
        state.arena.insert(decl_function(state.ast_arena, func_node))
    );

    // Veryify/test the return and parameter types.
    for (const t_node_id parameter : func_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter);

        t_symbol_id type_symbol;

        const node_type parameter_value_type = state.get_node_base_ptr(parameter_node.value_type)->type;
        const core::ast::node* default_value_base_node_ptr = state.get_node_base_ptr(parameter_node.default_value);

        const bool has_value_type = parameter_value_type != node_type::EXPR_NONE;
        const bool has_default_value = default_value_base_node_ptr->type != node_type::EXPR_NONE;

        if (!has_value_type) {
            type_symbol = deduce_expression_type(state, parameter_node.default_value);
        }

        if (has_default_value) {
            const t_symbol_id default_value_type_deduction = deduce_expression_type(state, parameter_node.default_value);

            if (!assert_types_match(state, parameter_node.selection, eval_type(state, parameter_node.value_type), default_value_type_deduction))
                continue;
        }
            
    }

    // Run the body.
    state.function_prescan_mode = true;
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
            eval_stmt_declaration(state, *(stmt_declaration*)node_base_ptr);
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