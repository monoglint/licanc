#pragma once

#include <string>

#include "frontend/sema/sym.hh"

#include "frontend/manager.hh"

namespace frontend::sema::full_passer {
    struct t_full_passer_context {

    };

    void full_pass(t_full_passer_context context);
}