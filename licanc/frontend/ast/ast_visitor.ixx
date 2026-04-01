/*

The AST visitor provides the base tools for implementing a recursive AST visitor.

Why not directly visit?
    The AST visitor provides solutions to cases such as an unresolved node type that hasn't been designed to be visited yet.

*/
module;

#include <concepts>

export module frontend.ast:ast_visitor;

import :ast_nodes;

export namespace frontend::ast {
    template <class Derived>
    class DeclVisitor {
    public:
        template <std::derived_from<Node> T>
        void visit_import(T& node) { 
            util::panic("AST node " + node.id);
        }
    };
}