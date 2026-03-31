module;

export module frontend.sema:ct_expr;

import util;

export namespace frontend::sema {
    struct CTExpr {};

    using CTExprIndex = util::SafeId<class CTExprIndexTag>;
    using CTExprPool = util::InternPool<CTExpr, CTExprIndex>;
};