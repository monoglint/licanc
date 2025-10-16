#include <string>
#include <algorithm>

#include "core.hh"
#include "ast.hh"
#include "symbol.hh"

using namespace core::sym;
using namespace core::ast;

struct semantic_state;

struct semantic_context {
    // If a specification is active, the following properties will be valid. This is when T is resolved to the type whatever called the function or instantiated the struct wants.
    t_symbol_id function_specification_id; // info_function_specification
    t_symbol_id struct_specification_id; // info_struct_specification

    // If we are in prescan mode
    t_symbol_id function_prescan_id; // decl_function        
    t_symbol_id struct_prescan_id; // decl_struct

    inline bool is_specification_open() const {
        return function_specification_id != 0 || struct_specification_id != 0;
    }

    inline bool is_prescan_open() const {
        return function_prescan_id != 0 || struct_prescan_id != 0;
    }
};

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

        // The context properties below equate to zero (technically the root's index) if they are not being used.

        semantic_context context;

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

static t_symbol_id eval_expr(semantic_state& state, const t_node_id node_id);
static void eval_stmt(semantic_state& state, const t_node_id node_id);
static void eval_item(semantic_state& state, const core::ast::t_node_id node_id);

/*

====================================================

Helper Functions

====================================================


*/

// Call asert_types_match() if you're only running to throw an error.
static bool types_match(semantic_state& state, const t_symbol_id type0, const t_symbol_id type1) {
    return type0 == type1;
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
static t_symbol_id _search_symbol_hierarchy(semantic_state& state, const decl_module& current_module, const t_node_id resolution_node_id) {
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
            focused_module = &state.get_symbol<decl_module>(_search_symbol_hierarchy(state, current_module, expr_binary_current_node.first));

        // Otherwise, the parse has syntactically guaranteed that the left side will be another binary expression.
        else {
            const t_symbol_id resolved_lhs = _search_symbol_hierarchy(state, current_module, expr_binary_current_node.first);
            
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


/*

Uses a specification's declaration to see if there is a template parameter whose name matches the given identifier. 
If there is a matching parameter, use its index to find which argument in the specification to return.

Specification symbol_id is just the id of the specification we are using to search the template parameters of.

*/
template <typename T_SPECIFICATION, typename T_DECLARATION>
// e.g.   'info_function_speci...'  decl_function
static t_symbol_id _check_specification_for_template_parameter_name(semantic_state& state, t_node_id specification_symbol_id, const core::t_identifier_id param_name) {
    const auto& declaration = state.get_symbol<T_DECLARATION>(state.get_symbol<T_SPECIFICATION>(specification_symbol_id).declaration);

    // expr_identifier can equate to t_identifier_ids. 
    const auto iterator = std::find(declaration.template_parameter_list.begin(), declaration.template_parameter_list.end(), param_name); 
    if (iterator == declaration.template_parameter_list.end())
        // We did not find a successful template parameter.
        return state.arena.insert(sym_invalid());

    const size_t index = iterator - declaration.template_parameter_list.begin();

    return state.get_symbol<T_SPECIFICATION>(specification_symbol_id).type_argument_list.at(index);
}

// Check if there is an active template parameter that is valid to the current prescanning context that matches the given identifier.
// If we find one, return a "temporary unresolved type" that is treated more leniently in type checking.
template <typename T_DECLARATION>
static t_symbol_id _check_prescan_for_template_parameters(semantic_state& state, t_node_id prescan_symbol_id, const core::t_identifier_id param_name) {
    const auto& declaration = state.get_symbol<T_DECLARATION>(prescan_symbol_id);
    const auto iterator = std::find(declaration.template_parameter_list.begin(), declaration.template_parameter_list.end(), param_name);

    if (iterator == declaration.template_parameter_list.end())
        return state.arena.insert(sym_invalid())

    return state.arena.insert(decl_unresolved_type());
}

/*

Given the provided identifier, look to see if it should be resolved based on the context of the analyzer;

If we find that a specification is open, attempt to find a specified symbol sourced from a template argument corresponding to the given identifier.
If we find that we are prescanning, then attempt to find a corresponding template parameter name and return a temporary symbol.

*/
static t_symbol_id _search_potential_prescans_and_specifications(semantic_state& state, const t_node_id identifier_node_id) {
    // Specified template arguments
    if (state.context.function_specification_id != 0) {
        const t_symbol_id searched_function_specialization_symbol = _check_specification_for_template_parameter_name<info_function_specification, decl_function>(
            state, 
            state.context.function_specification_id,
            state.get_node<expr_identifier>(identifier_node_id).id
        );

        if (state.arena.get_base_ptr(searched_function_specialization_symbol)->type != symbol_type::INVALID)
            return searched_function_specialization_symbol;
    }

    if (state.context.struct_specification_id != 0) {
        const t_symbol_id searched_struct_specialization_symbol = _check_specification_for_template_parameter_name<info_struct_specification, decl_struct>(
            state, 
            state.context.struct_specification_id,
            state.get_node<expr_identifier>(identifier_node_id).id
        );

        if (state.arena.get_base_ptr(searched_struct_specialization_symbol)->type != symbol_type::INVALID)
            return searched_struct_specialization_symbol;
    }

    // Unspecified template parameter matching
    if (state.context.function_prescan_id != 0) {
        const t_symbol_id searched
    }

    if (state.context.struct_prescan_id != 0) {

    }
}

static t_symbol_id search_symbol(semantic_state& state, const t_node_id resolution_node_id) {
    // Check if the symbol being accessed is controlled by the template system.
    // Basically any context that involves 'T' sourced from <T>
    if ((state.context.is_specification_open() || state.context.is_prescan_open()) && state.get_node_base_ptr(resolution_node_id)->type == node_type::EXPR_IDENTIFIER) {
        const t_symbol_id searched_symbol = _search_potential_prescans_and_specifications(state, resolution_node_id);

        if (state.arena.get_base_ptr(searched_symbol)->type != symbol_type::INVALID)
            return searched_symbol;
    }

    return _search_symbol_hierarchy(state, state.get_symbol<decl_module>(state.focused_module_id), resolution_node_id);
}

// Behaves like search_symbol, but is used in the context of creating a new declaration with its name being the last identifier in the tree.
static std::pair<decl_module&, core::t_identifier_id> search_symbol_for_naming(semantic_state& state, const t_node_id resolution_node) {
    // First check from the perspective of the current module

    // If the entry point identifier is not recognizable, 
    UNREACHABLE();
}

static t_symbol_id eval_expr_type(semantic_state& state, const t_node_id resolution_node_id) {
    const t_symbol_id found_symbol_id = search_symbol(state, resolution_node_id);
    const symbol* symbol_base_ptr = state.arena.get_base_ptr(found_symbol_id);
    
    switch (symbol_base_ptr->type) {
        case symbol_type::DECL_ENUM:
        case symbol_type::DECL_STRUCT:
        case symbol_type::DECL_PRIMITIVE:

        // search_symbol already casted its log if an error was made, so we'll include the invalid symbol type as a fallthrough condition.
        case symbol_type::INVALID:
            break;
        default:
            state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(resolution_node_id)->selection, "Invalid symbol - not a type.");
            return state.arena.insert(sym_invalid());
    }



    // TODO: Add type specifier stuff.
    return found_symbol_id;
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

// * Needs to work with specifications since this is statement-level
static void append_local_declaration(semantic_state& state, const t_node_id name_node_id, const t_symbol_id symbol_id) {

}

/*

====================================================

Tree walker functions

====================================================

*/

// Remember - operator overloads are prevalent here!
static t_symbol_id eval_expr_unary(semantic_state& state, const expr_unary& focus_node) {
    return eval_expr(state, focus_node.operand);
}

static t_symbol_id eval_expr_binary(semantic_state& state, const expr_binary& focus_node) {
    const t_node_id first_id = eval_expr(state, focus_node.first);
    const t_node_id second_id = eval_expr(state, focus_node.second);

    if (!assert_types_match(state, focus_node.selection, first_id, second_id)) {
        return state.arena.insert(sym_invalid());
    }

    return first_id;
}

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

    const t_symbol_id deduced_value_type = eval_expr(state, focus_node.value);

    if (state.get_node_base_ptr(focus_node.value_type)->type == node_type::EXPR_NONE)
        return;

    const t_symbol_id value_type_symbol = eval_expr_type(state, focus_node.value_type);
    assert_types_match(state, focus_node.selection, deduced_value_type, value_type_symbol);
}

static void eval_item_function_contents(semantic_state& state, const expr_function& func_node, t_symbol_id declaration_symbol_id) {
    semantic_context context_waypoint = state.context;
    state.context = semantic_context {

    };
    
    // Veryify/test the return and parameter types.
    for (const t_node_id parameter : func_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter);

        t_symbol_id type_symbol;

        const node_type parameter_value_type = state.get_node_base_ptr(parameter_node.value_type)->type;
        const core::ast::node* default_value_base_node_ptr = state.get_node_base_ptr(parameter_node.default_value);

        const bool has_value_type = parameter_value_type != node_type::EXPR_NONE;
        const bool has_default_value = default_value_base_node_ptr->type != node_type::EXPR_NONE;

        if (!has_value_type) {
            type_symbol = eval_expr(state, parameter_node.default_value);
        }

        if (has_default_value) {
            const t_symbol_id default_value_type_deduction = eval_expr(state, parameter_node.default_value);

            if (!assert_types_match(state, parameter_node.selection, eval_expr_type(state, parameter_node.value_type), default_value_type_deduction))
                continue;
        }   
    }

    // Prescan the body. Contexts should be null at this point.
    eval_stmt(state, func_node.body);
    
    state.context = context_waypoint;
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& focus_node) {
    const expr_function& func_node = state.get_node<expr_function>(focus_node.function);

    t_symbol_id declaration_symbol_id = state.arena.insert(decl_function(state.ast_arena, func_node));

    append_item_declaration(
        state,
        focus_node.source,
        declaration_symbol_id
    );

    eval_item_function_contents(state, func_node, declaration_symbol_id);
}

static void eval_item_module(semantic_state& state, const item_module& focus_node) {
    t_symbol_id parent_module_id = state.focused_module_id;
    t_symbol_id new_module_id = state.arena.insert(decl_module());

    state.focused_module_id = new_module_id;
    eval_item(state, focus_node.content);
    state.focused_module_id = parent_module_id;
}

static void eval_item_use(semantic_state& state, const item_use& focus_node) {
    state.process.add_file(state.get_node<expr_literal>(focus_node.path).read(state.process));
}

static t_symbol_id eval_expr(semantic_state& state, const t_node_id node_id) {
    const core::ast::node* node_base_ptr = state.get_node_base_ptr(node_id); 

    switch (node_base_ptr->type) {
        case node_type::EXPR_UNARY:
            return eval_expr_unary(state, *(expr_unary*)node_base_ptr);
        default:
            break;
    }

    UNREACHABLE();
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
        case core::ast::node_type::ITEM_USE:
            eval_item_use(state, *(core::ast::item_use*)node_base_ptr);
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