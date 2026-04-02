module;

#include <string>

export module frontend.ast:ast_pools;

import util;

export namespace frontend::ast {
    using IdentifierId = util::SafeId<class IdentifierIdTag>;
    using IdentifierPool = util::InternPool<std::string, IdentifierId>;

    using StringLiteralId = util::SafeId<class StringLiteralIdTag>;
    using StringLiteralPool = util::InternPool<std::string, StringLiteralId>;
}