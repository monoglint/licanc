#include <string>

#include "core.hh"
#include "ast.hh"
#include "symbol.hh"

/*



get back to note

update ast.hh and parser.cc to separate scope resolution, assignment, and access expressions from expr_binary into their own nodes


*/

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

    t_symbol_id focused_module_id;

    bool semantic_success = true;

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
}

// Covers all declarations. Appends basically any symbol into the currently active namespaces.
static void append_declaration(semantic_state& state, const core::lisel& selection, const t_node_id resolution_node_id, const t_symbol_id symbol_id) {
    auto& focused_module = state.get_symbol<decl_module>(state.focused_module_id);

    // Locate the module we want to declare the variable in RELATIVE TO focused_module

    std::pair<decl_module&, core::t_identifier_id> name_info = search_symbol_for_naming(state, state.get_node<expr_binary>(resolution_node_id));
    auto& target_module = name_info.first;
    auto& name = name_info.second;

    if (target_module.declarations.find(name) != target_module.declarations.end()) {
        state.add_log(core::lilog::log_level::ERROR, selection, "\"" + state.process.identifier_lookup.get(name) + "\" was already declared.");
        return;
    }

    target_module.declarations[name] = symbol_id;
}

/*

====================================================

*/

// Local variable declaration
static void eval_expr_declaration(semantic_state& state, const expr_declaration& node) {

}

// Global variable declaration
static void eval_item_declaration(semantic_state& state, const item_declaration& node) {
    append_declaration(
        state,
        node.selection,
        state.get_node<expr_identifier>(node.name).id,
        state.arena.insert(decl_variable(node.value_type))
    );

    eval_expr(state, node.value);
}

static void eval_item_function_declaration(semantic_state& state, const item_function_declaration& node) {
    append_declaration(
        state,
        node.selection,
        state.get_node<expr_identifier>(node.name).id,
        state.arena.insert(decl_function(state.ast_arena, node.function))
    );

    eval_
}

static void eval_item_module(semantic_state& state, const item_module& node) {
    t_symbol_id parent_module_id = state.focused_module_id;
    t_symbol_id new_module_id = state.arena.insert(decl_module());
    state.focused_module_id = new_module_id;
    
    eval_item(state, node.content);

    state.focused_module_id = parent_module_id;
}

static void eval_expr(semantic_state& state, const t_node_id node_id) {
    const core::ast::node* base_node = state.get_node_base_ptr(node_id); 

    switch (base_node->type) {
        default:
            break;
    }
}

static void eval_stmt(semantic_state& state, const t_node_id node_id) {
    const core::ast::node* base_node = state.get_node_base_ptr(node_id); 
    
    switch (base_node->type) {
        case core::ast::node_type::EXPR_DECLARATION:
            eval_expr_declaration(state, *(expr_declaration*)base_node);
            break;
        default:
            break;
    }
}

static void eval_item(semantic_state& state, const core::ast::t_node_id node_id) {
    const core::ast::node* base_node = state.get_node_base_ptr(node_id); 
    
    switch (base_node->type) {
        case core::ast::node_type::ITEM_MODULE:
            eval_item_module(state, *(core::ast::item_module*)base_node);
            break;
        case core::ast::node_type::ITEM_DECLARATION:
            eval_item_declaration(state, *(core::ast::item_declaration*)base_node);
            break;
        case core::ast::node_type::ITEM_FUNCTION_DECLARATION:
            eval_item_function_declaration(state, *(core::ast::item_function_declaration*)base_node);
            break;
        default:
            break;
    }

    state.process.add_log(
        core::lilog::log_level::COMPILER_ERROR,
        base_node->selection,
        "Unexpected AST node - Expected 'item', got [code " + std::to_string((uint8_t)base_node->type) + "])"
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