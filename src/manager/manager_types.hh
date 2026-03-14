#pragma once

#include "util/safe_id.hh"

/*

NOTE: ALL OF THESE ID TYPES ARE std::size_t BECAUSE std::size_t IS THE TYPE USED FOR INDEXES IN MOST C++ CONTAINERS.
DO NOT DOWNCAST THESE TYPES, THEY SHOULD REMAIN std::size_t

*/


namespace manager {
    // index of FrontendFile in FrontendUnit::files
    using FileId = util::SafeId<struct FileIdTag>;

    // index of std::string in FrontendUnit::identifier_pool
    using IdentifierId = util::SafeId<struct IdentifierIdTag>;

    // index of std::string in FrontendUnit::string_literal_pool
    using StringLiteralId = util::SafeId<struct StringLiteralIdTag>;

    using ResolvedTypeId = util::SafeId<struct ResolvedTypeIdTag>;

    using ConstexprId = util::SafeId<struct ConstexprIdTag>;
};