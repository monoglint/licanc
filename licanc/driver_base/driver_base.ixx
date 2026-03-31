module;

#include <string>

export module driver_base;

import util;

export namespace driver_base {
    // index of std::string in FrontendUnit::identifier_pool
    using IdentifierIndex = util::SafeId<class IdentifierIdTag>;
    using IdentifierPool = util::InternPool<std::string, IdentifierIndex>;

    // index of std::string in FrontendUnit::string_literal_pool
    using StringLiteralIndex = util::SafeId<class StringLiteralIdTag>;
    using StringLiteralPool = util::InternPool<std::string, StringLiteralIndex>;
};