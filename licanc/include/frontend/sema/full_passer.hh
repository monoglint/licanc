#pragma once

#include <string>

#include "frontend/sema/sym.hh"

#include "frontend/manager.hh"

namespace frontend::sema::full_passer {
    struct FullPasserContext {

    };

    void full_pass(FullPasserContext context);
}