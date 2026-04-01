/*

The ast builder is designed to be used by the parser or ast writer to construct ASTs that abide by certain rules.

@monoglint
Last updated: 30 March 2026

*/

/*

---===AST CONSTRUCTION RULES===---

A violation of these rules deems an AST as malformed.

1. A child node can not refer to its parent node.
2. Every node must be identifiable by ID.

---===EXPLANATIONS===---

1. If a child node directly references a parent node, a recursion loop can be caused for the visitor.

2. Assigning a unique ID to every AST node will simplify serializing and deserializing (more specifically, converting memory references into
    IDs, and vice versa). The serializer and deserializer need IDs for nodes to reference each other because a file format can not use
    memory addresses as safe data. 

---===ENFORCEMENT===---

1. Currently, it is impossible or unknown how I should implement a clean runtime mechanism that ensures no circular-dependencies inside of the ast_nodes file.
    Therefore, it should be by design that the construction of nodes in this file do not allow recursive referencing.

2. ID creation is implicitly managed by the AST builder.

*/

module;

#include <concepts>

export module frontend.ast:ast_builder;

import :ast_nodes;
import :ast_class;

export namespace frontend::ast {
    class ASTBuilder {
        ASTBuilder(AST& tree)
            : tree(tree)
        {}

        AST& tree;

        template <typename T>
        requires std::derived_from<T, Node>
        T push(T node) {
            
        }
    };
}