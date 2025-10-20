#include "symbol.hh"
#include "ast.hh"

using namespace core::sym;

void _pretty_debug(const core::liprocess& process, const symbol_arena& sym_arena, const core::ast::ast_arena& ast_arena, const t_symbol_id id, std::string& buffer, std::unordered_set<t_symbol_id>& ignore_set, uint8_t indent) {
    const symbol* base_ptr = sym_arena.get_base_ptr(id);

    if (ignore_set.find(id) != ignore_set.end())
        return;

    ignore_set.insert(id);
    
    switch (base_ptr->type) {
        case symbol_type::SPEC_FUNCTION:
        case symbol_type::SPEC_PRIMITIVE: {
            const specification* as_spec = (specification*)sym_arena.get_base_ptr(id);
            buffer += liutil::indent_repeat(indent) + sym_arena.get_symbol_name(process, as_spec->declaration_id) + '\n';
            break;
        }

        case symbol_type::TYPE_WRAPPER: {
            const auto& as_type_wrapper = sym_arena.get_as<type_wrapper>(id);
            buffer += liutil::indent_repeat(indent) + "[qual] ";
            _pretty_debug(process, sym_arena, ast_arena, sym_arena.unwrap_type_wrapper(as_type_wrapper), buffer, ignore_set, 0);

            break;
        }
        case symbol_type::INVALID:
            buffer += "<invalid node>\n";
            break;
        default:
            buffer += "<unknown node>\n";
    }
}

std::string core::sym::symbol_arena::pretty_debug(const core::liprocess& process, const core::ast::ast_arena& ast_arena, const t_symbol_id id) {
    std::string buffer;
    std::unordered_set<t_symbol_id> ignore_set;

    _pretty_debug(process, *this, ast_arena, id, buffer, ignore_set, 0);

    return buffer;
}