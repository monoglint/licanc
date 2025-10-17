#include <string>
#include <algorithm>

#include "core.hh"
#include "ast.hh"
#include "symbol.hh"

using namespace core::sym;
using namespace core::ast;

struct semantic_state;

constexpr t_symbol_id INVALD_SYMBOL_ID = 0;
constexpr t_symbol_id ROOT_SYMBOL_ID = 1;
constexpr t_symbol_id GLOBAL_MODULE_SYMBOL_ID = 2;
// semantic context
enum class semcon {
    FUNC,
    STRUCT,
};

struct semantic_context {
    // If a specification is active, the following properties will be valid. This is when T is resolved to the type whatever called the function or instantiated the struct wants.
    t_symbol_id function_specification_id; // info_function_specification
    t_symbol_id struct_specification_id; // info_struct_specification

    // If we are in prescan mode
    t_symbol_id function_prescan_id; // decl_function        
    t_symbol_id struct_prescan_id; // decl_struct

    inline bool is_specification_open() const {
        return function_specification_id != INVALD_SYMBOL_ID || struct_specification_id != INVALD_SYMBOL_ID;
    }

    inline bool is_prescan_open() const {
        return function_prescan_id != INVALD_SYMBOL_ID || struct_prescan_id != INVALD_SYMBOL_ID;
    }

    template <semcon CONTEXT_TYPE>
    inline void set_specification(const t_symbol_id specification_id) {
        if constexpr (CONTEXT_TYPE == semcon::FUNC) {
            function_specification_id = specification_id;
            function_prescan_id = INVALD_SYMBOL_ID;
        }
        else {
            struct_specification_id = specification_id;
            struct_prescan_id = INVALD_SYMBOL_ID;
        }
    }

    template <semcon CONTEXT_TYPE>
    inline void set_prescan(const t_symbol_id prescan_id) {
        if constexpr (CONTEXT_TYPE == semcon::FUNC) {
            function_specification_id = INVALD_SYMBOL_ID;
            function_prescan_id = prescan_id;
        }
        else {
            struct_specification_id = INVALD_SYMBOL_ID;
            struct_prescan_id = prescan_id;
        }
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

// This function should take in either .
static bool types_match(semantic_state& state, const type_wrapper& type0, const type_wrapper& type1) {
    // Any unspecified type should not be type checked no matter the qualifiers.
    if (type0.wrapee_id == INVALD_SYMBOL_ID || type1.wrapee_id == INVALD_SYMBOL_ID)
        return true;

    if (type0.qualifier != type1.qualifier)
        return false;

    auto wrapee0 = state.arena.get_base_ptr(type0.wrapee_id);
    auto wrapee1 = state.arena.get_base_ptr(type1.wrapee_id); 
        
    if (wrapee0->type != wrapee1->type)
        return false;
    
    if (wrapee0->type == symbol_type::TYPE_WRAPPER)
        return types_match(state, state.get_symbol<type_wrapper>(type0.wrapee_id), state.get_symbol<type_wrapper>(type1.wrapee_id));

    // i forget why this is here
    return wrapee0 == wrapee1;
}

// Potentially add casting later? Not necessary.
static bool assert_types_match(semantic_state& state, const core::lisel& error_selection, const type_wrapper type0, const type_wrapper type1) {
    if (!types_match(state, type0, type1)) {
        // !NOTE - We could totally add a flag that makes the type checker optional. Holy C community would love this
        state.add_log(core::lilog::log_level::ERROR, error_selection, "Types do not match. Double check sources and qualifiers.");
        return false;
    }

    return true;
}

static bool assert_types_match(semantic_state& state, const core::lisel& error_selection, const t_node_id type0, const t_node_id type1) {
    return assert_types_match(state, error_selection, state.get_symbol<type_wrapper>(type0), state.get_symbol<type_wrapper>(type1));
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
        auto& global_module = state.get_symbol<decl_module>(state.arena.get_as<sym_root>(ROOT_SYMBOL_ID).global_module);
        
        if (global_module.has_item(identifier_node.id)) {
            return global_module.declaration_map.at(identifier_node.id);
        }

        // Okay, fallback unsuccessful; we can confidently assert that the programmer fucked up in some way, shape, or form.
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + identifier_node.read(state.process) + "\" was not declared in this scope.");
        return INVALD_SYMBOL_ID;
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
                return INVALD_SYMBOL_ID;
            }

            focused_module = &state.get_symbol<decl_module>(resolved_lhs);
        }

    // Resolve RHS

    const expr_identifier& rhs_identifier_node = state.get_node<expr_identifier>(expr_binary_current_node.second);
     
    if (!focused_module->has_item(rhs_identifier_node.id)) {
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + rhs_identifier_node.read(state.process) + "\" was not declared in this scope.");
        return INVALD_SYMBOL_ID;
    }

    return focused_module->declaration_map.at(rhs_identifier_node.id);
}


/*

Uses a specification's declaration to see if there is a template parameter whose name matches the given identifier. 
If there is a matching parameter, use its index to find which argument in the specification to return.

*/
static t_symbol_id _fill_potential_template_parameter_with_argument(semantic_state& state, t_node_id specification_symbol_id, const core::t_identifier_id potential_param_name) {
    const specification* specification_symbol = (specification*)state.get_node_base_ptr(specification_symbol_id);
    const specifiable* declaration_symbol = (specifiable*)state.arena.get_base_ptr(specification_symbol->declaration_id);

    const auto iterator = std::find(declaration_symbol->template_parameter_list.begin(), declaration_symbol->template_parameter_list.end(), potential_param_name);

    if (iterator == declaration_symbol->template_parameter_list.end())
        return INVALD_SYMBOL_ID;

    const size_t iterator_index = iterator - declaration_symbol->template_parameter_list.begin();

    return specification_symbol->template_argument_list.at(iterator_index);
}

// Check if there is an active template parameter that is valid to the current prescanning context that matches the given identifier.
// If we find one, return a "temporary unresolved type" that is treated more leniently in type checking.
static t_symbol_id _check_prescan_for_potential_template_parameter(semantic_state& state, t_node_id prescan_symbol_id, const core::t_identifier_id potential_param_name) {
    const specifiable* declaration_symbol = (specifiable*)(state.arena.get_base_ptr(prescan_symbol_id));

    const auto iterator = std::find(declaration_symbol->template_parameter_list.begin(), declaration_symbol->template_parameter_list.end(), potential_param_name);

    if (iterator == declaration_symbol->template_parameter_list.end())
        return INVALD_SYMBOL_ID;

    return state.arena.insert(type_wrapper(INVALD_SYMBOL_ID, core::e_type_qualifier::NONE));
}

/*

Given the provided identifier, look to see if or how it should be resolved based on the context of the analyzer;

If we find that a specification is open, attempt to find a specified symbol sourced from a template argument corresponding to the given identifier.
If we find that we are prescanning, then attempt to find a corresponding template parameter name and return a temporary symbol.

Return the specified or unspecified type wrapper.

->type_wrapper | invalid
*/
static t_symbol_id _search_potential_prescans_and_specifications(semantic_state& state, const t_node_id identifier_node_id) {
    const core::t_identifier_id potential_param_name = state.get_node<expr_identifier>(identifier_node_id).id;

    // Specified template arguments
    if (state.context.function_specification_id != INVALD_SYMBOL_ID) {
        const t_symbol_id searched_function_specialization_symbol = _fill_potential_template_parameter_with_argument(state, state.context.function_specification_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_function_specialization_symbol)->type != symbol_type::INVALID)
            return searched_function_specialization_symbol;
    }

    if (state.context.struct_specification_id != INVALD_SYMBOL_ID) {
        const t_symbol_id searched_struct_specialization_symbol = _fill_potential_template_parameter_with_argument(state, state.context.struct_specification_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_struct_specialization_symbol)->type != symbol_type::INVALID)
            return searched_struct_specialization_symbol;
    }

    // Unspecified template parameter matching
    if (state.context.function_prescan_id != INVALD_SYMBOL_ID) {
        const t_symbol_id searched_function_prescan_symbol = _check_prescan_for_potential_template_parameter(state, state.context.function_prescan_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_function_prescan_symbol)->type != symbol_type::INVALID)
            return searched_function_prescan_symbol;
    }

    if (state.context.struct_prescan_id != INVALD_SYMBOL_ID) {
        const t_symbol_id searched_function_prescan_symbol = _check_prescan_for_potential_template_parameter(state, state.context.function_prescan_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_function_prescan_symbol)->type != symbol_type::INVALID)
            return searched_function_prescan_symbol;
    }

    return INVALD_SYMBOL_ID;
}

/*

dec foo[T]() {
    dec x: T = 5
}

foo[int]()

*/

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

static t_symbol_id find_or_generate_specification(semantic_state& state, const t_symbol_id declaration_id, const t_node_list& template_argument_list);
// -> type_wrapper | invalid
static t_symbol_id eval_expr_type(semantic_state& state, const t_node_id focus_node_id) {
    const expr_type& focus_node = state.get_node<expr_type>(focus_node_id);

    const node* type_source_base_ptr = state.get_node_base_ptr(focus_node.source);

    if (type_source_base_ptr->type == node_type::EXPR_TYPE) {
        const core::e_type_qualifier qualifier = focus_node.qualifier;

        return state.arena.insert(type_wrapper(eval_expr_type(state, focus_node.source), qualifier));
    }

    // Under this 'implicit else' condition, focus_node.source is an identifier or binary expression.

    const t_symbol_id found_symbol_id = search_symbol(state, focus_node.source);
    const symbol* symbol_base_ptr = state.arena.get_base_ptr(found_symbol_id);
    
    switch (symbol_base_ptr->type) {
        case symbol_type::DECL_ENUM:
        case symbol_type::DECL_STRUCT:
        case symbol_type::DECL_PRIMITIVE:
        case symbol_type::DECL_TYPEDEC: {
            // Create a specification and its wrapper.

            t_node_id specification = find_or_generate_specification(state, found_symbol_id, state.get_node<expr_type>(focus_node_id).argument_list);
            return state.arena.insert(type_wrapper(specification, state.get_node<expr_type>(focus_node_id).qualifier));
        }

        case symbol_type::TYPE_WRAPPER: {
            return found_symbol_id;
        }

        // return early instead of erroring - search_symbol already errored for us under this condition
        case symbol_type::INVALID:
            return found_symbol_id;
        default:
            state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(focus_node_id)->selection, "Invalid symbol - not a type.");
            return state.arena.insert(sym_invalid());
    }

    return state.arena.insert(type_wrapper(found_symbol_id, core::e_type_qualifier::NONE));
}

// vector<eval_expr_type(type_node_N)>
static t_symbol_list eval_type_list(semantic_state& state, const t_node_list& type_node_list) {
    t_symbol_list type_symbol_list;

    for (const auto& type_id : type_node_list) {
        type_symbol_list.push_back(eval_expr_type(state, type_id));
    }

    return type_symbol_list;
}

// -> info_function_specification
static t_symbol_id generate_function_specification(semantic_state& state, const t_symbol_id declaration_id, const t_node_list& template_argument_node_list) {
    const auto& declaration = state.get_symbol<decl_function>(declaration_id);
    
    const t_symbol_id return_type_symbol_id = eval_expr_type(state, declaration.node.return_type);
    const t_symbol_list template_argument_symbol_list = eval_type_list(state, template_argument_node_list);
    
    const t_symbol_id specification_symbol_id = state.arena.insert(info_function_specification(return_type_symbol_id, template_argument_symbol_list, declaration_id));

    state.get_symbol<decl_function>(declaration_id).specification_map.insert({template_argument_symbol_list, specification_symbol_id});

    semantic_context context_waypoint = state.context;
    state.context.set_specification<semcon::FUNC>(specification_symbol_id);
    eval_stmt(state, state.get_symbol<decl_function>(declaration_id).node.body);    
    state.context = context_waypoint;

    return specification_symbol_id;
}

// -> info_..._specification
static t_symbol_id find_or_generate_specification(semantic_state& state, const t_symbol_id declaration_id, const t_node_list& template_argument_list) {
    const specifiable* as_specifiable = (specifiable*)state.arena.get_base_ptr(declaration_id);
    const auto iterator = as_specifiable->specification_map.find(template_argument_list);

    if (iterator != as_specifiable->specification_map.end())
        return iterator->second;

    if (as_specifiable->type == symbol_type::DECL_FUNCTION)
        return generate_function_specification(state, declaration_id, template_argument_list);

    // Haven't gotten to this implementation yet
    STMT_UNREACHABLE("find_or_generate_specification");
}

// Behaves like search_symbol, but is used in the context of creating a new declaration with its name being the last identifier in the tree.
static std::pair<decl_module&, core::t_identifier_id> search_symbol_for_naming(semantic_state& state, const t_node_id resolution_node) {
    // First check from the perspective of the current module

    // If the entry point identifier is not recognizable, 
    STMT_UNREACHABLE("search_symbol_for_naming");
}

// Covers all declarations. Appends basically any symbol into the currently active namespaces.
static void append_item_declaration(semantic_state& state, const t_node_id resolution_node_id, const t_symbol_id symbol_id) {
    // Locate the module we want to declare the variable in RELATIVE TO focused_module

    std::pair<decl_module&, core::t_identifier_id> name_info = search_symbol_for_naming(state, resolution_node_id);
    auto& target_module = name_info.first;
    auto& name_id = name_info.second;

    if (target_module.declaration_map.find(name_id) != target_module.declaration_map.end()) {
        state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(resolution_node_id)->selection, "\"" + state.process.identifier_lookup.get(name_id) + "\" was already declared.");
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
        return INVALD_SYMBOL_ID;
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

static void prescan_function_decl(semantic_state& state, const expr_function& func_node, t_symbol_id declaration_symbol_id) {
    semantic_context context_waypoint = state.context;
    state.context.set_prescan<semcon::FUNC>(declaration_symbol_id);
    
    // Veryify/test the return and parameter types.
    for (const t_node_id parameter : func_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter);

        t_symbol_id type_symbol;
        
        // (x: foo = bar)
        //            ^
        
        const core::ast::node* default_value_base_node_ptr = state.get_node_base_ptr(parameter_node.default_value);

        const bool has_value_type = state.get_node_base_ptr(parameter_node.value_type)->type != node_type::EXPR_NONE;
        const bool has_default_value = default_value_base_node_ptr->type != node_type::EXPR_NONE;

        // The following processes are just for type checking essentially.

        // (x: foo)
        //      ^ <-- if that exists
        if (has_value_type)
            // Veryify that the type semantics are true. Pass forward for potential type checking with default value.
            type_symbol = eval_expr_type(state, parameter_node.value_type);    
        else
            type_symbol = eval_expr(state, parameter_node.default_value);         

        if (has_default_value) {
            const t_symbol_id default_value_type_deduction = eval_expr(state, parameter_node.default_value);

            if (!assert_types_match(state, parameter_node.selection, type_symbol, default_value_type_deduction))
                continue;
        }   

        // Case down here is we have an evaluated type and no default value. Nothing we gotta do! No type checking.
    }

    // Prescan the body. Contexts should be null at this point.
    eval_stmt(state, func_node.body);
    
    state.context = context_waypoint;
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& focus_node) {
    const expr_function& func_node = state.get_node<expr_function>(focus_node.function);

    t_symbol_id declaration_symbol_id = state.arena.insert(decl_function(func_node));

    std::cout << "Append declaration\n";
    append_item_declaration(
        state,
        focus_node.source,
        declaration_symbol_id
    );

    std::cout << "Prescan func decl\n";
    prescan_function_decl(state, func_node, declaration_symbol_id);
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

    STMT_UNREACHABLE("eval_expr");
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
            state.process.add_log(
                core::lilog::log_level::COMPILER_ERROR,
                node_base_ptr->selection,
                "Unexpected AST node - Expected 'item', got [code " + std::to_string((uint8_t)node_base_ptr->type) + "])"
            );
            break;
    }
}

static void eval_ast_root(semantic_state& state, const core::ast::ast_root& node) {
    for (const core::ast::t_node_id& child : node.item_list) {
        eval_item(state, child);
    }
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);

    state.arena.insert(sym_invalid()); // INVALD_SYMBOL_ID
    state.arena.insert(sym_root()); // ROOT_SYMBOL_ID
    state.arena.insert(decl_module()); // GLOBAL_MODULE_SYMBOL_ID

    state.get_symbol<sym_root>(ROOT_SYMBOL_ID).global_module = GLOBAL_MODULE_SYMBOL_ID;
    state.focused_module_id = GLOBAL_MODULE_SYMBOL_ID;

    //eval_ast_root(state, state.get_node<core::ast::ast_root>(0));

    //std::cout << "Finished successfully\n";

    return true;
}