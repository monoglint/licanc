module;

#include <string>

export module driver_base;

import util;

export namespace driver_base {
    // index of std::string in FrontendUnit::identifier_pool
    using IdentifierId = util::SafeId<class IdentifierIdTag>;
    using IdentifierPool = util::InternPool<std::string, IdentifierId>;

    // index of std::string in FrontendUnit::string_literal_pool
    using StringLiteralId = util::SafeId<class StringLiteralIdTag>;
    using StringLiteralPool = util::InternPool<std::string, StringLiteralId>;

    // before you come here and do it, future me, 
};