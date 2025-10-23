#include <string>
#include <algorithm>

#include "sema.hh"
#include "core.hh"
#include "ast.hh"
#include "symbol.hh"

using namespace core::sym;
using namespace core::ast;
using namespace core::sema;

struct semantic_context {
    // If a specification is active, the following properties will be valid. This is when T is resolved to the type whatever called the function or instantiated the struct wants.
    t_symbol_id function_specification_id = SYM_INVALID_ID; // info_function_specification
    t_symbol_id struct_specification_id = SYM_INVALID_ID; // info_struct_specification

    // If we are in prescan mode
    t_symbol_id function_prescan_id = SYM_INVALID_ID; // decl_function        
    t_symbol_id struct_prescan_id = SYM_INVALID_ID; // decl_struct

    inline bool is_specification_open() const {
        return function_specification_id != SYM_INVALID_ID || struct_specification_id != SYM_INVALID_ID;
    }

    inline bool is_prescan_open() const {
        return function_prescan_id != SYM_INVALID_ID || struct_prescan_id != SYM_INVALID_ID;
    }

    inline void dump() const {
        std::cout << "Func spec: " << function_specification_id << '\n';
        std::cout << "Stru spec: " << struct_specification_id << '\n';
        std::cout << "Func pres: " << function_prescan_id << '\n';
        std::cout << "Stru pres: " << struct_prescan_id << '\n';
    }

    inline void operator=(const semantic_context& other) {
        // std::cout << "Reset context\n";
        function_specification_id = other.function_specification_id;
        struct_specification_id = other.struct_specification_id;
        function_prescan_id = other.function_prescan_id;
        struct_prescan_id = other.struct_prescan_id;
    }

    template <semcon CONTEXT_TYPE>
    inline void set_specification(const t_symbol_id specification_id) {
        if constexpr (CONTEXT_TYPE == semcon::FUNC) {
            function_specification_id = specification_id;
            function_prescan_id = SYM_INVALID_ID;
        }
        else {
            struct_specification_id = specification_id;
            struct_prescan_id = SYM_INVALID_ID;
        }
    }

    template <semcon CONTEXT_TYPE>
    inline void set_prescan(const t_symbol_id prescan_id) {
        if constexpr (CONTEXT_TYPE == semcon::FUNC) {
            function_specification_id = SYM_INVALID_ID;
            function_prescan_id = prescan_id;
        }
        else {
            struct_specification_id = SYM_INVALID_ID;
            struct_prescan_id = prescan_id;
        }
    }
};

struct local {
    local(const core::t_identifier_id name, const t_symbol_id value_type)
        : name(name), value_type(value_type) {}

    core::t_identifier_id name;
    t_symbol_id value_type; // type_wrapper
};

struct call_frame {
    // Stacked based locals are just a compile-time abstraction.
    // Locals will be stored in temp "buckets" or registers in runtime.
    std::vector<local> local_stack; 
};

using t_call_stack = std::vector<call_frame>;

struct semantic_state {
    semantic_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process),
          file_id(file_id),
          ast_arena(std::any_cast<const core::ast::ast_arena&>(process.file_list[file_id].dump_ast_arena))
          {}

    core::liprocess& process;

    const core::t_file_id file_id;

    inline core::liprocess::lifile& file() const {
        return process.file_list[file_id];
    }

    // safe; no longer changed at this point in runtime
    const ast_arena& ast_arena;
    symbol_arena arena;
    
    t_call_stack call_stack;

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

    inline const node* get_node_base_ptr(const t_node_id id) const {
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
static void eval_item(semantic_state& state, const t_node_id node_id);

/*

====================================================

Helper Functions

====================================================


*/

// This function should take in either .
static bool types_match(semantic_state& state, const type_wrapper& type0, const type_wrapper& type1) {
    const symbol* wrapee0_base_ptr = state.arena.get_base_ptr(type0.wrapee_id);
    const symbol* wrapee1_base_ptr = state.arena.get_base_ptr(type1.wrapee_id);

    // Compare qualifier of current layer
    if (type0.qualifier != type1.qualifier)
        return false;

    // If we're not at the lowest layer...
    if (wrapee0_base_ptr->type == symbol_type::TYPE_WRAPPER && wrapee1_base_ptr->type == symbol_type::TYPE_WRAPPER)
        return types_match(state, *(type_wrapper*)wrapee0_base_ptr, *(type_wrapper*)wrapee1_base_ptr);

    // If we are at the lowest layer, compare the specifications we are pointing to.
    return type0.wrapee_id == type1.wrapee_id || type0.wrapee_id == SYM_INVALID_ID || type1.wrapee_id == SYM_INVALID_ID;
    //                                              If any of the two types are unspecified, just say they match.
}

// Potentially add casting later? Not necessary.
static bool assert_types_match(semantic_state& state, const core::lisel& error_selection, const t_symbol_id type0_id, const t_symbol_id type1_id) {
    if (!types_match(state, state.get_symbol<type_wrapper>(type0_id), state.get_symbol<type_wrapper>(type1_id))) {
        // !NOTE - We could totally add a flag that makes the type checker optional. Holy C community would love this
        const std::string type0_str = state.arena.pretty_debug(state.process, state.ast_arena, type0_id);
        const std::string type1_str = state.arena.pretty_debug(state.process, state.ast_arena, type1_id);

        state.add_log(core::lilog::log_level::ERROR, error_selection, "Type mismatch\n" + type0_str + type1_str);
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
        auto& global_module = state.get_symbol<decl_module>(state.arena.get_as<sym_root>(SYM_ROOT_ID).global_module);
        
        if (global_module.has_item(identifier_node.id)) {
            return global_module.declaration_map.at(identifier_node.id);
        }

        // Okay, fallback unsuccessful; we can confidently assert that the programmer fucked up in some way, shape, or form.
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + identifier_node.read(state.process) + "\" was not declared in this scope.");
        return SYM_INVALID_ID;
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
                return SYM_INVALID_ID;
            }

            focused_module = &state.get_symbol<decl_module>(resolved_lhs);
        }

    // Resolve RHS

    const expr_identifier& rhs_identifier_node = state.get_node<expr_identifier>(expr_binary_current_node.second);
     
    if (!focused_module->has_item(rhs_identifier_node.id)) {
        state.add_log(core::lilog::log_level::ERROR, current_node->selection, "\"" + rhs_identifier_node.read(state.process) + "\" was not declared in this scope.");
        return SYM_INVALID_ID;
    }

    return focused_module->declaration_map.at(rhs_identifier_node.id);
}

// Behaves like search_symbol, but is used in the context of creating a new declaration with its name being the last identifier in the tree.
static std::pair<decl_module&, core::t_identifier_id> search_symbol_for_naming(semantic_state& state, decl_module& current_module, const t_node_id resolution_node_id) {
    const node* resolution_node_base_ptr = state.get_node_base_ptr(resolution_node_id);

    if (resolution_node_base_ptr->type == node_type::EXPR_IDENTIFIER) {
        return { current_module, state.get_node<expr_identifier>(resolution_node_id).id };
    }

    // Otherwise, we're in a bianry expression.

    const auto& expr_binary_resolution_node = state.get_node<expr_binary>(resolution_node_id);
    t_symbol_id parent_module = _search_symbol_hierarchy(state, current_module, expr_binary_resolution_node.first);

    return { state.get_symbol<decl_module>(parent_module), state.get_node<expr_identifier>(expr_binary_resolution_node.second).id };
}

/*

Uses a specification's declaration to see if there is a template parameter whose name matches the given identifier. 
If there is a matching parameter, use its index to find which argument in the specification to return.

*/
static t_symbol_id _fill_potential_template_parameter_with_argument(semantic_state& state, const t_symbol_id specification_symbol_id, const core::t_identifier_id potential_param_name) {
    const specification* specification_symbol = (specification*)state.arena.get_base_ptr(specification_symbol_id);
    const specifiable* declaration_symbol = (specifiable*)state.arena.get_base_ptr(specification_symbol->declaration_id);
    const auto iterator = std::find_if(
        declaration_symbol->template_parameter_list.get().begin(),
        declaration_symbol->template_parameter_list.get().end(),
        [&state, &potential_param_name](const t_node_id focus_node) { return state.get_node<expr_identifier>(focus_node).id == potential_param_name; }
    );

    if (iterator == declaration_symbol->template_parameter_list.get().end())
        return SYM_INVALID_ID;

    const size_t iterator_index = iterator - declaration_symbol->template_parameter_list.get().begin();

    return specification_symbol->template_argument_list.at(iterator_index);
}

// Check if there is an active template parameter that is valid to the current prescanning context that matches the given identifier.
// If we find one, return a "temporary unresolved type" that is treated more leniently in type checking.
static t_symbol_id _check_decl_for_potential_template_parameter(semantic_state& state, const t_symbol_id decl_symbol_id, const core::t_identifier_id potential_param_name) {
    const specifiable* declaration_symbol = (specifiable*)(state.arena.get_base_ptr(decl_symbol_id));

    const auto iterator = std::find_if(
        declaration_symbol->template_parameter_list.get().begin(),
        declaration_symbol->template_parameter_list.get().end(),
        [&state, &potential_param_name](const t_node_id focus_node) { return state.get_node<expr_identifier>(focus_node).id == potential_param_name; }
    );

    if (iterator == declaration_symbol->template_parameter_list.get().end()) {
        return SYM_INVALID_ID;
    }   

    return state.arena.insert(type_wrapper(SYM_INVALID_ID));
}

static bool is_resolution_node_a_template_parameter_name(semantic_state& state, const t_symbol_id decl_symbol_id, const t_node_id resolution_node) {
    if (state.get_node_base_ptr(resolution_node)->type != node_type::EXPR_IDENTIFIER)
        return false;

    return _check_decl_for_potential_template_parameter(state, decl_symbol_id, state.get_node<expr_identifier>(resolution_node).id);
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
    if (state.context.function_specification_id != SYM_INVALID_ID) {
        const t_symbol_id searched_function_specialization_symbol = _fill_potential_template_parameter_with_argument(state, state.context.function_specification_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_function_specialization_symbol)->type != symbol_type::INVALID)
            return searched_function_specialization_symbol;
    }

    if (state.context.struct_specification_id != SYM_INVALID_ID) {
        const t_symbol_id searched_struct_specialization_symbol = _fill_potential_template_parameter_with_argument(state, state.context.struct_specification_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_struct_specialization_symbol)->type != symbol_type::INVALID)
            return searched_struct_specialization_symbol;
    }

    // Unspecified template parameter matching
    if (state.context.function_prescan_id != SYM_INVALID_ID) {
        const t_symbol_id searched_function_prescan_symbol = _check_decl_for_potential_template_parameter(state, state.context.function_prescan_id, potential_param_name);
        if (state.arena.get_base_ptr(searched_function_prescan_symbol)->type != symbol_type::INVALID)
            return searched_function_prescan_symbol;
    }

    if (state.context.struct_prescan_id != SYM_INVALID_ID) {
        const t_symbol_id searched_function_prescan_symbol = _check_decl_for_potential_template_parameter(state, state.context.struct_prescan_id, potential_param_name);

        if (state.arena.get_base_ptr(searched_function_prescan_symbol)->type != symbol_type::INVALID)
            return searched_function_prescan_symbol;
    }

    return SYM_INVALID_ID;
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

static t_symbol_id find_or_generate_specification(semantic_state& state, const t_symbol_id declaration_id, const core::lisel& error_selection, const t_node_list& template_argument_node_list);
// -> type_wrapper | invalid
// Only call under the condition that focus_node is a type. Resolve NONE externally.
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

            t_symbol_id specification = find_or_generate_specification(state, found_symbol_id, state.get_node_base_ptr(focus_node_id)->selection, state.get_node<expr_type>(focus_node_id).argument_list);
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

    return state.arena.insert(type_wrapper(found_symbol_id));
}

// vector<eval_expr_type(type_node_N)>
static t_symbol_list eval_expr_type_list(semantic_state& state, const t_node_list& type_node_list) {
    t_symbol_list type_symbol_list;

    for (const auto& type_id : type_node_list) {
        type_symbol_list.push_back(eval_expr_type(state, type_id));
    }

    return type_symbol_list;
}

static bool check_decl_function_parameters(semantic_state& state, const expr_function& func_node);

// -> info_function_specification
static t_symbol_id generate_function_specification(semantic_state& state, const t_symbol_id declaration_id, const t_node_list& template_argument_node_list) {
    t_symbol_list template_argument_symbol_list = eval_expr_type_list(state, template_argument_node_list);

    for (const t_symbol_id symbol_id : template_argument_symbol_list) {
        if (symbol_id == SYM_INVALID_ID)
            return SYM_INVALID_ID;
    }

    const t_symbol_id specification_symbol_id = state.arena.insert(spec_function(std::move(template_argument_symbol_list), declaration_id));
    state.get_symbol<decl_function>(declaration_id).specification_map.insert({template_argument_node_list, specification_symbol_id});

    semantic_context context_waypoint = state.context;
    state.context.set_specification<semcon::FUNC>(specification_symbol_id);

    check_decl_function_parameters(state, state.get_symbol<decl_function>(declaration_id).node.get());

    const t_symbol_id return_type_symbol_id = eval_expr_type(state, state.get_symbol<decl_function>(declaration_id).node.get().return_type);

    state.get_symbol<spec_function>(specification_symbol_id).return_type_id = return_type_symbol_id;

    state.context = context_waypoint;

    return specification_symbol_id;
}

static t_symbol_id generate_primitive_specification(semantic_state& state, const t_symbol_id declaration_id) {
    const t_symbol_id specification_symbol_id = state.arena.insert(spec_primitive(declaration_id));

    auto& declaration = state.get_symbol<decl_primitive>(declaration_id);

    std::pair<const core::sym::t_symbol_list, const core::sym::t_symbol_id> pair = {declaration._empty_template_parameter_list, specification_symbol_id}; 
    // even though you're suppossed to insert arguments here, template_parameter_list is always empty. just use it instead. it has the same type anyway
    declaration.specification_map.insert(std::move(pair));

    return specification_symbol_id;
}

// -> info_..._specification
static t_symbol_id find_or_generate_specification(semantic_state& state, const t_symbol_id declaration_id, const core::lisel& error_selection, const t_node_list& template_argument_node_list) {
    const specifiable* as_specifiable = (specifiable*)state.arena.get_base_ptr(declaration_id);

    const size_t argument_count = template_argument_node_list.size();
    const size_t parameter_count = as_specifiable->template_parameter_list.get().size();

    if (argument_count != parameter_count) {
        state.add_log(core::lilog::log_level::ERROR, error_selection, "Expected " + std::to_string(parameter_count) + " template argument(s), got " + std::to_string(argument_count) +".");
        return SYM_INVALID_ID;
    }

    // Ensure that no type arguments are unspecified
    if (state.context.function_prescan_id != SYM_INVALID_ID) {
        for (const t_node_id arg_id : template_argument_node_list) {
            const expr_type& as_type_arg_node = state.get_node<expr_type>(arg_id);
            const t_node_id unwrapped = state.ast_arena.unwrap_expr_type(as_type_arg_node);
            
            if (state.get_node_base_ptr(unwrapped)->type != node_type::EXPR_IDENTIFIER)
                continue;
 
            const t_symbol_id checked_param = _check_decl_for_potential_template_parameter(
                state, 
                state.context.function_prescan_id, 
                state.get_node<expr_identifier>(unwrapped).id
            );
            
            // One of the template arguments is unspecified.
            if (checked_param != SYM_INVALID_ID)
                return SYM_INVALID_ID;
        }
    }

    const auto iterator = as_specifiable->specification_map.find(template_argument_node_list);

    if (iterator != as_specifiable->specification_map.end())
        return iterator->second;

    if (as_specifiable->type == symbol_type::DECL_FUNCTION)
        return generate_function_specification(state, declaration_id, template_argument_node_list);
    else if (as_specifiable->type == symbol_type::DECL_PRIMITIVE) {
        if (template_argument_node_list.size() > 0)
            state.add_log(core::lilog::log_level::COMPILER_ERROR, state.get_node_base_ptr(template_argument_node_list[0])->selection, "Primitives can not have specifications.");
        return generate_primitive_specification(state, declaration_id);
    }
        
    state.add_log(core::lilog::log_level::COMPILER_ERROR, error_selection, "Unexpected specification type to generate.");
    return SYM_INVALID_ID;
}

// Covers all declarations. Appends basically any symbol into the currently active namespaces.
static void append_item_declaration(semantic_state& state, const t_node_id resolution_node_id, const t_symbol_id symbol_id) {
    // Locate the module we want to declare the variable in RELATIVE TO focused_module

    std::pair<decl_module&, core::t_identifier_id> name_info = search_symbol_for_naming(state, state.get_symbol<decl_module>(state.focused_module_id), resolution_node_id);
    auto& target_module = name_info.first;
    auto& name_node_id = name_info.second;

    if (target_module.declaration_map.find(name_node_id) != target_module.declaration_map.end()) {
        state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(resolution_node_id)->selection, "\"" + state.process.identifier_lookup.get(name_node_id) + "\" was already declared in this module.");
        return;
    }

    target_module.declaration_map.insert({name_node_id, symbol_id});
    state.arena.symbol_name_map.insert({symbol_id, name_node_id});
}

// symbol_id is for a type wrapper
static void append_local_declaration(semantic_state& state, const t_node_id name_node_id, const t_symbol_id type_symbol_id) {
    call_frame& frame = state.call_stack.back();

    const core::t_identifier_id identifier_id = state.get_node<expr_identifier>(name_node_id).id;

    auto iterator = std::find_if(
        frame.local_stack.begin(),
        frame.local_stack.end(),
        [&state, &identifier_id](const local& loc) { return loc.name == identifier_id; } 
    ); 

    if (iterator != frame.local_stack.end()) {
        state.add_log(core::lilog::log_level::ERROR, state.get_node_base_ptr(name_node_id)->selection, "\"" + state.process.identifier_lookup.get(identifier_id) + "\" was already declared in this scope.");
        return;
    }

    frame.local_stack.emplace_back(name_node_id, type_symbol_id);
}

static t_symbol_id eval_expr_parameter(semantic_state& state, const expr_parameter& focus_node);

// Assume that we are already in the correct context.
// Function assumes function's parameter and argument count are already the same.
static void call_function(semantic_state& state, const expr_function& function_node, const expr_call& call_node) {   
    state.call_stack.emplace_back();

    for (size_t i = 0; i < call_node.argument_list.size(); i++) {
        const t_node_id argument_id = call_node.argument_list[i];
        const t_symbol_id argument_type = eval_expr(state, argument_id);

        const t_node_id parameter_id = function_node.parameter_list[i];
        const auto& parameter_node = state.get_node<expr_parameter>(parameter_id);
        const t_symbol_id parameter_type = eval_expr_parameter(state, parameter_node);

        // Parameter_type could be invalid. For quick recovery, still run the other function.
        if (parameter_type != SYM_INVALID_ID)
            assert_types_match(state, state.get_node_base_ptr(argument_id)->selection, parameter_type, argument_type);
            
        append_local_declaration(state, parameter_node.name, argument_type);
    }

    eval_stmt(state, function_node.body);

    state.call_stack.pop_back();
}

static void call_function_as_prescan(semantic_state& state, const expr_function& declaration_node) {
    state.call_stack.emplace_back();

    for (const t_node_id parameter_id : declaration_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter_id);

        t_symbol_id parameter_type = eval_expr_parameter(state, parameter_node);

        state.call_stack.back().local_stack.emplace_back(
            state.get_node<expr_identifier>(parameter_node.name).id,
            parameter_type
        );
    }

    eval_stmt(state, declaration_node.body);

    state.call_stack.pop_back();
}

/*

====================================================

Tree walker functions

====================================================

*/

static t_symbol_id eval_expr_parameter(semantic_state& state, const expr_parameter& focus_node) {
    if (state.get_node_base_ptr(focus_node.value_type)->type != node_type::EXPR_NONE)
        return eval_expr_type(state, focus_node.value_type);
    else
        return eval_expr(state, focus_node.default_value);
}

// Remember - operator overloads are prevalent here!
static t_symbol_id eval_expr_unary(semantic_state& state, const expr_unary& focus_node) {
    return eval_expr(state, focus_node.operand);
}

static t_symbol_id eval_expr_binary(semantic_state& state, const expr_binary& focus_node) {
    const t_node_id first_id = eval_expr(state, focus_node.first);
    const t_node_id second_id = eval_expr(state, focus_node.second);

    if ((first_id == SYM_INVALID_ID || second_id == SYM_INVALID_ID) || !assert_types_match(state, focus_node.selection, first_id, second_id)) {
        return SYM_INVALID_ID;
    }

    return first_id;
}

static t_symbol_id eval_expr_literal(semantic_state& state, const expr_literal& focus_node) {
    switch (focus_node.literal_type) {
        case expr_literal::e_literal_type::INT: {
            return state.arena.insert(type_wrapper(find_or_generate_specification(state, SYM_TI32_ID, focus_node.selection, {})));
        }
        case expr_literal::e_literal_type::FLOAT: {
            return state.arena.insert(type_wrapper(find_or_generate_specification(state, SYM_TF32_ID, focus_node.selection, {})));
        }
        default:    
            break;
    }

    state.add_log(core::lilog::log_level::COMPILER_ERROR, focus_node.selection, "Unhandled literal type.");
    return SYM_INVALID_ID;
}

static t_symbol_id eval_expr_call(semantic_state& state, const expr_call& focus_node) {
    const t_symbol_id declaration_symbol_id = search_symbol(state, focus_node.callee);

    switch (state.arena.get_base_ptr(declaration_symbol_id)->type) {
        case symbol_type::INVALID:
            return SYM_INVALID_ID;
        case symbol_type::DECL_FUNCTION:
            break;
        default:
            state.add_log(core::lilog::log_level::ERROR, focus_node.selection, "Attempted to call a symbol that is not a function.");
            return SYM_INVALID_ID;
    }

    t_symbol_id specification_symbol_id = find_or_generate_specification(state, declaration_symbol_id, focus_node.selection, focus_node.template_argument_list);

    if (specification_symbol_id == SYM_INVALID_ID)
        return SYM_INVALID_ID;

    semantic_context context_waypoint = state.context;
    state.context.set_specification<semcon::FUNC>(specification_symbol_id);

    call_function(state, state.get_symbol<decl_function>(declaration_symbol_id).node.get(), focus_node);

    // Return type doesn't have to be checked since it was in the prescan.

    state.context = context_waypoint;
    
    return state.get_symbol<spec_function>(specification_symbol_id).return_type_id;
}

static t_symbol_id eval_expr_identifier(semantic_state& state, const expr_identifier& focus_node) {
    state.add_log(core::lilog::log_level::ERROR, focus_node.selection, focus_node.read(state.process) + " is unavailable.");
    return SYM_INVALID_ID;
}

// Local variable declaration
static void eval_stmt_declaration(semantic_state& state, const stmt_declaration& focus_node) {
    append_local_declaration(
        state,
        focus_node.name,
        state.arena.insert(decl_variable(focus_node.value_type))
    );

    const t_symbol_id deduced_value_type = eval_expr(state, focus_node.value);

    if (deduced_value_type == SYM_INVALID_ID || state.get_node_base_ptr(focus_node.value_type)->type == node_type::EXPR_NONE)
        return;

    const t_symbol_id value_type_symbol = eval_expr_type(state, focus_node.value_type);
    assert_types_match(state, focus_node.selection, deduced_value_type, value_type_symbol);
}

static void eval_stmt_return(semantic_state& state, const stmt_return& focus_node) {
    t_symbol_id return_value_type_symbol_id = eval_expr(state, focus_node.expression);

    if (return_value_type_symbol_id == SYM_INVALID_ID)
        return;
    
    // We assue that if a specification isn't open, then a prescan is.
    t_symbol_id& function_return_type_symbol_id = 
        state.context.function_specification_id != SYM_INVALID_ID 
            ? state.get_symbol<spec_function>(state.context.function_specification_id).return_type_id
            : state.get_symbol<decl_function>(state.context.function_prescan_id).return_type_id;

    // Function return type was declared implicit. Therefore, we don't need to type check.
    if (function_return_type_symbol_id == SYM_INVALID_ID) {
        function_return_type_symbol_id = return_value_type_symbol_id;
        return;
    }

    // There is an explicit function return type.
    assert_types_match(
        state, 
        state.get_node_base_ptr(focus_node.expression)->selection, 
        return_value_type_symbol_id,
        function_return_type_symbol_id
    );
}

static void eval_stmt_compound(semantic_state& state, const stmt_compound& focus_node) {
    for (const t_node_id subnode_id : focus_node.stmt_list) {
        eval_stmt(state, subnode_id);
    }
}

// Global variable declaration
static void eval_item_declaration(semantic_state& state, const item_declaration& focus_node) {
    append_item_declaration(
        state,
        focus_node.source,
        state.arena.insert(decl_variable(focus_node.value_type))
    );

    const t_symbol_id deduced_value_type = eval_expr(state, focus_node.value);

    if (deduced_value_type == SYM_INVALID_ID || state.get_node_base_ptr(focus_node.value_type)->type == node_type::EXPR_NONE)
        return;

    const t_symbol_id value_type_symbol = eval_expr_type(state, focus_node.value_type);
    assert_types_match(state, focus_node.selection, deduced_value_type, value_type_symbol);
}

// Works in any context
static bool check_decl_function_parameters(semantic_state& state, const expr_function& func_node) {
    // Veryify/test the return and parameter types.
    for (const t_node_id parameter : func_node.parameter_list) {
        const auto& parameter_node = state.get_node<expr_parameter>(parameter);

        t_symbol_id type_symbol;

        const node* default_value_base_node_ptr = state.get_node_base_ptr(parameter_node.default_value);

        const bool has_value_type = state.get_node_base_ptr(parameter_node.value_type)->type != node_type::EXPR_NONE;
        const bool has_default_value = default_value_base_node_ptr->type != node_type::EXPR_NONE;

        if (has_value_type) {
            // If we are in a specification, we only want to validate the types the prescanner couldn't get.
            if (
                state.context.function_specification_id != SYM_INVALID_ID && !is_resolution_node_a_template_parameter_name(
                    state, 
                    state.get_symbol<spec_function>(state.context.function_specification_id).declaration_id,
                    state.ast_arena.unwrap_expr_type(state.get_node<expr_type>(parameter_node.value_type))
                )
            )
                continue;

            // Verify that the type semantics are true. Pass forward for potential type checking with default value.
            type_symbol = eval_expr_type(state, parameter_node.value_type); 
        }
        else
            type_symbol = eval_expr(state, parameter_node.default_value);  

        if (has_default_value) {
            const t_symbol_id default_value_type_deduction = eval_expr(state, parameter_node.default_value);

            if (
                (type_symbol == SYM_INVALID_ID || default_value_type_deduction == SYM_INVALID_ID)
                || !assert_types_match(state, state.get_node_base_ptr(parameter_node.default_value)->selection, type_symbol, default_value_type_deduction)
            )
                return false;
        }   

        // Case down here is we have an evaluated type and no default value. Nothing we gotta do! No type checking.
    }

    return true;
}

static void prescan_function_decl(semantic_state& state, const expr_function& func_node, t_symbol_id declaration_symbol_id) {
    semantic_context context_waypoint = state.context;
    state.context.set_prescan<semcon::FUNC>(declaration_symbol_id);
    
    check_decl_function_parameters(state, func_node);

    t_symbol_id return_type_id = eval_expr_type(state, func_node.return_type);

    // This can be unspecified since we're in the prescan. It's all good.
    state.get_symbol<decl_function>(declaration_symbol_id).return_type_id = return_type_id;

    // Prescan the body. Contexts should be null at this point.
    call_function_as_prescan(state, func_node);
    
    state.context = context_waypoint;
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& focus_node) {
    const expr_function& func_node = state.get_node<expr_function>(focus_node.function);
    t_symbol_id declaration_symbol_id = state.arena.insert(decl_function(func_node));

    append_item_declaration(
        state,
        focus_node.source,
        declaration_symbol_id
    );

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
    const node* node_base_ptr = state.get_node_base_ptr(node_id); 

    switch (node_base_ptr->type) {
        case node_type::EXPR_UNARY:
            return eval_expr_unary(state, *(expr_unary*)node_base_ptr);
        case node_type::EXPR_BINARY:
            return eval_expr_binary(state, *(expr_binary*)node_base_ptr);
        case node_type::EXPR_CALL:
            return eval_expr_call(state, *(expr_call*)node_base_ptr);
        case node_type::EXPR_LITERAL:
            return eval_expr_literal(state, *(expr_literal*)node_base_ptr);
        case node_type::EXPR_IDENTIFIER:
            return eval_expr_identifier(state, *(expr_identifier*)node_base_ptr);
        default:
            break;
    }

    state.add_log(core::lilog::log_level::COMPILER_ERROR, node_base_ptr->selection, "Unexpected expression type.");
    return SYM_INVALID_ID;
}

static void eval_stmt(semantic_state& state, const t_node_id node_id) {
    const node* node_base_ptr = state.get_node_base_ptr(node_id); 
    
    switch (node_base_ptr->type) {
        case node_type::STMT_DECLARATION:
            eval_stmt_declaration(state, *(stmt_declaration*)node_base_ptr);
            break;
        case node_type::STMT_RETURN:
            eval_stmt_return(state, *(stmt_return*)node_base_ptr);
            break;
        case node_type::STMT_COMPOUND:
            eval_stmt_compound(state, *(stmt_compound*)node_base_ptr);
            break;
        case node_type::EXPR_CALL:
            eval_expr_call(state, *(expr_call*)node_base_ptr);
            break;
        default:
            break;
    }
}

static void eval_item(semantic_state& state, const t_node_id node_id) {
    const node* node_base_ptr = state.get_node_base_ptr(node_id); 
    
    switch (node_base_ptr->type) {
        case node_type::ITEM_MODULE:
            eval_item_module(state, *(item_module*)node_base_ptr);
            break;
        case node_type::ITEM_DECLARATION:
            eval_item_declaration(state, *(item_declaration*)node_base_ptr);
            break;
        case node_type::ITEM_FUNCTION_DECLARATION:
            eval_item_function_declaration(state, *(item_function_declaration*)node_base_ptr);
            break;
        case node_type::ITEM_USE:
            eval_item_use(state, *(item_use*)node_base_ptr);
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

static void eval_ast_root(semantic_state& state, const ast_root& node) {
    for (const t_node_id& child : node.item_list) {
        eval_item(state, child);
    }
}

static void soft_module_insert(semantic_state& state, decl_module& module, const std::string name, const t_symbol_id symbol_id) {
    const core::t_identifier_id identifier_id = state.process.identifier_lookup.insert(name);
    module.declaration_map.insert({identifier_id, symbol_id});
    state.arena.symbol_name_map.insert({symbol_id, identifier_id});
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);

    state.arena.insert(sym_invalid()); // SYM_INVALID_ID
    state.arena.insert(sym_root()); // SYM_ROOT_ID
    state.arena.insert(decl_module()); // SYM_GLOBAL_MODULE_ID

    state.arena.insert(decl_primitive(4, 4)); // i32
    state.arena.insert(decl_primitive(4, 4)); // f32

    auto& root_symbol = state.get_symbol<sym_root>(SYM_ROOT_ID);
    root_symbol.global_module = SYM_GLOBAL_MODULE_ID;

    auto& global_module_symbol = state.get_symbol<decl_module>(SYM_GLOBAL_MODULE_ID);
    soft_module_insert(state, global_module_symbol, "i32", SYM_TI32_ID);
    soft_module_insert(state, global_module_symbol, "f32", SYM_TF32_ID);

    state.focused_module_id = SYM_GLOBAL_MODULE_ID;

    eval_ast_root(state, state.get_node<ast_root>(0));

    // const auto& global_module_symbol = state.get_symbol<decl_module>(SYM_GLOBAL_MODULE_ID);
    // const auto iterator = state.process.identifier_lookup.get_id("main");

    // if (state.process.identifier_lookup.is_get_id_iterator_valid(iterator)) { 
    //     auto declaration_iterator = global_module_symbol.declaration_map.find(iterator->second);

    //     if (declaration_iterator != global_module_symbol.declaration_map.end()) {
            
    //         (state.get_symbol<decl_function>(declaration_iterator->second).specification_map.at({}));
    //     }
    // }

    state.file().dump_symbol_table = std::any(std::move(state.arena));

    return true;
}